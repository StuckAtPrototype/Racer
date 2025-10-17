/**
 * @file main.c
 * @brief Main application entry point for the Racer3 firmware
 * 
 * This file contains the main application logic for the Racer3 autonomous racing car.
 * It initializes all subsystems including BLE, motors, LEDs, color sensors, and battery monitoring.
 * The main loop handles color detection, game state management, and battery monitoring.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "motor.h"
#include <math.h>

// BLE (Bluetooth Low Energy) subsystem
#include "gap.h"
#include "gatt_svr.h"
#include "nvs_flash.h"
#include "esp_bt.h"

// Addressable LED control subsystem
#include "ws2812_control.h"
#include "led_color_lib.h"
#include "led.h"

// Battery monitoring subsystem
#include "battery.h"

// GPIO interrupt handling for color sensor triggers
#include "gpio_interrupt.h"

// I2C configuration for color sensor communication
#include "i2c_config.h"

// Color sensor and prediction subsystems
#include "tcs3400.h"
#include "color_predictor.h"
#include "controller.h"
#include "ring_buffer_rgb.h"



// LEDC (LED Controller) configuration for motor PWM control
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT  // 10-bit duty resolution (0-1023)
#define LEDC_FREQUENCY          (15000)            // 15 kHz PWM frequency

// Motor GPIO pin assignments
#define MOTOR_A_FWD_GPIO        13  // Motor A forward direction pin
#define MOTOR_A_BWD_GPIO        14  // Motor A backward direction pin
#define MOTOR_B_FWD_GPIO        4   // Motor B forward direction pin
#define MOTOR_B_BWD_GPIO        5   // Motor B backward direction pin

// Global variables
static bool motor_direction[NUM_MOTORS] = {true, true};  // Motor direction state (true=forward, false=backward)

QueueHandle_t gpio_intr_evt_queue = NULL;  // Queue for GPIO interrupt events
//NeuralNetwork nn;  // Neural network for color prediction (currently unused)

ring_buffer_t rbg_buffer;  // Ring buffer for RGB color data averaging

// Battery monitoring variables
static bool low_battery_warning = false;      // Low battery warning flag
static uint32_t battery_check_counter = 0;    // Counter for battery check timing

// Function prototypes
void gpio_interrupt_task(void *pvParameters);
void rbg_read(void);

/**
 * @brief Convert RGB color values to hue value
 * 
 * This function converts RGB color values from the OPT4060 color sensor to a hue value
 * in the range 0-65535, which is compatible with the LED color library.
 * 
 * @param red Red component value from color sensor
 * @param green Green component value from color sensor  
 * @param blue Blue component value from color sensor
 * @param clear Clear channel value (unused in this function)
 * @return uint16_t Hue value in range 0-65535
 */
uint16_t opt4060_to_hue(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
    float r_norm, g_norm, b_norm;
    float max, min, hue;

    // Normalize RGB values to the range [0, 1] by dividing by the maximum component
    max = (float)(red > green ? (red > blue ? red : blue) : (green > blue ? green : blue));
    min = (float)(red < green ? (red < blue ? red : blue) : (green < blue ? green : blue));

    r_norm = (float)red / max;
    g_norm = (float)green / max;
    b_norm = (float)blue / max;

    // Calculate hue based on normalized RGB values using standard HSV conversion
    if (max == min) {
        // Grayscale case: Hue is undefined, default to 0
        hue = 0.0f;
    } else if (max == r_norm) {
        hue = 60.0f * ((g_norm - b_norm) / (max - min));
    } else if (max == g_norm) {
        hue = 60.0f * (2.0f + ((b_norm - r_norm) / (max - min)));
    } else { // max == b_norm
        hue = 60.0f * (4.0f + ((r_norm - g_norm) / (max - min)));
    }

    // Ensure hue is positive (handle negative values)
    if (hue < 0.0f) {
        hue += 360.0f;
    }

    // Scale to 0-65535 range as expected by get_color_from_hue function
    return (uint16_t)(hue * 65535.0f / 360.0f);
}


/**
 * @brief Main application entry point
 * 
 * Initializes all subsystems including:
 * - LEDC timer for motor PWM control
 * - LED control system
 * - BLE (Bluetooth Low Energy) communication
 * - Motor control system
 * - Battery monitoring
 * - Color sensor and GPIO interrupt handling
 * 
 * Then enters the main application loop which handles:
 * - Battery voltage monitoring
 * - Color detection and game state management
 * - LED status updates
 */
void app_main(void)
{
    // Configure LEDC timer for motor PWM control
    ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_MODE,
            .timer_num        = LEDC_TIMER,
            .duty_resolution  = LEDC_DUTY_RES,
            .freq_hz          = LEDC_FREQUENCY,
            .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        printf("LEDC Timer Config failed: %s\n", esp_err_to_name(err));
        return;
    }

    // Initialize LED control system
    led_init();
    led_set_flash_period(pdMS_TO_TICKS(100));  // Set default flash period to 100ms

    // Initialize BLE (Bluetooth Low Energy) subsystem
    nimble_port_init();
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.reset_cb = reset_cb;
    gatt_svr_init();
    ble_svc_gap_device_name_set(device_name);
    nimble_port_freertos_init(host_task);


    // Configure motor PWM channels for each motor direction
    configure_motor_pwm(MOTOR_A_FWD_GPIO, LEDC_CHANNEL_0);  // Motor A forward
    configure_motor_pwm(MOTOR_A_BWD_GPIO, LEDC_CHANNEL_1);  // Motor A backward
    configure_motor_pwm(MOTOR_B_FWD_GPIO, LEDC_CHANNEL_2);  // Motor B forward
    configure_motor_pwm(MOTOR_B_BWD_GPIO, LEDC_CHANNEL_3);  // Motor B backward

    // Create motor control queues for each motor
    motor_queue[0] = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(MotorUpdate));
    if (motor_queue[0] == NULL) {
        printf("Failed to create motor queue for Motor A\n");
        return;
    }
    motor_queue[1] = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(MotorUpdate));
    if (motor_queue[1] == NULL) {
        printf("Failed to create motor queue for Motor B\n");
        return;
    }

    // Create semaphore for synchronizing motor start operations
    motor_start_semaphore = xSemaphoreCreateCounting(4, 0);
    if (motor_start_semaphore == NULL) {
        printf("Failed to create motor start semaphore\n");
        return;
    }

    // Create motor control tasks for each motor
    xTaskCreate(motor_task, "motor_task_A", 2048, (void *)0, 1, NULL);
    xTaskCreate(motor_task, "motor_task_B", 2048, (void *)1, 1, NULL);

    // Initialize the motor controller subsystem
    controller_init();

    // Initialize battery monitoring system
    esp_err_t ret = battery_init();
    if (ret != ESP_OK) {
        ESP_LOGE("main", "Battery initialization failed");
        return;
    }

    // Initialize neural network for color prediction (currently unused)
    //initialize_neural_network(&nn);

    // Initialize ring buffer for RGB color data averaging
    ring_buffer_init(&rbg_buffer);

    // Initialize GPIO interrupt system for color sensor triggers
    configure_gpio_interrupt();
    gpio_intr_evt_queue = gpio_interrupt_get_evt_queue();
    xTaskCreate(gpio_interrupt_task, "gpio_interrupt_task", 2048, NULL, 10, NULL);

    // Initialize TCS3400 color sensor
    TCS3400_init();



    // Main application loop
    while (1) {
        // Battery monitoring: Check voltage every 10 iterations (every 100ms)
        battery_check_counter++;
        if (battery_check_counter >= 10) {
            battery_check_counter = 0;
            
            float voltage;
            esp_err_t ret = battery_read_voltage(&voltage);
            if (ret == ESP_OK) {
                ESP_LOGI("main", "Battery ADC counts: %d", (int)voltage);
                
                // Check for low battery condition (threshold: 3000 ADC counts)
                if (voltage < 3000) {
                    if (!low_battery_warning) {
                        ESP_LOGW("main", "LOW BATTERY WARNING: %d counts", (int)voltage);
                        low_battery_warning = true;
                    }
                } else {
                    if (low_battery_warning) {
                        ESP_LOGI("main", "Battery voltage recovered: %d counts", (int)voltage);
                        low_battery_warning = false;
                    }
                }
            } else {
                ESP_LOGE("main", "Failed to read battery voltage");
            }
        }
        
        // Update LED indicator based on battery status
        if (low_battery_warning) {
            led_set_indicator_color(LED_COLOR_RED);
            led_set_flash_mode(LED_FLASH_INDICATOR);
            led_set_flash_period(pdMS_TO_TICKS(200));  // Flash every 200ms for low battery
        }
        // When battery is normal, LED control is handled by game state system

        // Read RGB color data from sensor
        rbg_read();

        // Color processing and classification (currently disabled)
        // These commented sections show potential color enhancement techniques:
        // - Gamma correction for brightness adjustment
        // - Saturation enhancement
        // - Color classification and LED control
        
        // Apply gamma correction and increase saturation
        //red_norm = powf(red_norm, 1.8);   // Gamma < 1 brightens
        //green_norm = powf(green_norm, 1.8);
        //blue_norm = powf(blue_norm, 1.8);

        // Re-normalize after gamma correction
        //total = red_norm + green_norm + blue_norm;
        //red_norm = red_norm / total;
        //green_norm = green_norm / total;
        //blue_norm = blue_norm / total;

        // Color classification and LED control
        //process_and_classify_color(red_byte, green_byte, blue_byte, clear);

        // Main loop delay: 10ms (100Hz update rate)
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


/**
 * @brief Read RGB color data from TCS3400 sensor and store in ring buffer
 * 
 * This function reads normalized RGB color values from the TCS3400 color sensor
 * and stores them in the ring buffer for averaging. The sensor returns values
 * normalized to the clear channel for consistent color detection.
 */
void rbg_read(void){
    uint32_t red, green, blue, clear;
    
    // Read normalized color values from TCS3400 sensor
    TCS3400_read_color(&red, &green, &blue, &clear);

    // Color correction factors (currently disabled)
    // These can be used to compensate for sensor characteristics:
    // red *= 2.4;    // Red channel gain
    // green *= 1.0;  // Green channel gain (reference)
    // blue *= 1.3;   // Blue channel gain

    // Color normalization and conversion (currently disabled)
    // This code shows how to convert normalized values to 8-bit RGB:
    // float total = red + green + blue;
    // float red_norm = red / total;
    // float green_norm = green / total;
    // float blue_norm = blue / total;
    // uint8_t red_byte = (uint8_t)(red_norm * 255);
    // uint8_t green_byte = (uint8_t)(green_norm * 255);
    // uint8_t blue_byte = (uint8_t)(blue_norm * 255);

    // Store RGB values in ring buffer for averaging
    ring_buffer_put(&rbg_buffer, red, green, blue);

    // Debug logging (currently disabled)
    //ESP_LOGI("main", "RGB Values: R:%u, G:%u, B:%u, Clear:%lu",
    //         red_byte, green_byte, blue_byte, clear);
}

/**
 * @brief GPIO interrupt task for handling color sensor triggers
 * 
 * This task processes GPIO interrupts from the color sensor and performs
 * color detection and game state management. When an interrupt occurs,
 * it reads multiple color samples, averages them, classifies the color,
 * and updates the game state accordingly.
 * 
 * @param pvParameters Task parameters (unused)
 */
void gpio_interrupt_task(void *pvParameters)
{
    uint32_t io_num;
    for(;;) {
        // Wait for GPIO interrupt event
        if(xQueueReceive(gpio_intr_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI("main","GPIO[%lu] interrupt occurred!", io_num);

            // Visual feedback for interrupt detection (currently disabled)
            // led_set_taillight_color(LED_COLOR_YELLOW);

            // Small delay to allow sensor to stabilize after interrupt
            // Note: Ideally this should be refactored to avoid delays in interrupt context
            vTaskDelay(pdMS_TO_TICKS(10)); 
            
            // Clear any stale readings by taking an initial sample
            rbg_read();
            
            // Take multiple samples for averaging (4 additional samples)
            for(int i = 0; i < 4; i++){
                vTaskDelay(pdMS_TO_TICKS(3));  // 3ms between samples
                rbg_read();
            }

            uint32_t red, green, blue, clear;
            
            // Alternative sensor reading (currently disabled)
            //opt4060_read_color(&red, &green, &blue, &clear);
            //
            // Neural network color prediction (currently disabled)
            //uint32_t color = predict_color(&nn, red, green, blue, clear);
            //
            // Color correction factors for OPT4060 sensor
            //red *= 2.4;
            //green *= 1.0;
            //blue *= 1.3;
            //
            // Color normalization and conversion
            //float total = red + green + blue;
            //float red_norm = red / total;
            //float green_norm = green / total;
            //float blue_norm = blue / total;
            //uint8_t red_byte = (uint8_t)(red_norm * 255);
            //uint8_t green_byte = (uint8_t)(green_norm * 255);
            //uint8_t blue_byte = (uint8_t)(blue_norm * 255);
            //
            // Note: Color processing should ideally be done outside interrupt context
            // and run at constant intervals, but triggered by interrupt events

            // Get averaged RGB values from ring buffer
            ring_buffer_get_avg(&rbg_buffer, &red, &green, &blue);

            // Classify the detected color
            color_class_t color = classify_color_rgb(red, green, blue, 0);

            ESP_LOGI("main", "Average RGB Values: R:%lu, G:%lu, B:%lu color: %s",
                     red, green, blue, get_color_name(color));

            // Update game state based on detected color
            command_set_game_status(color);

            // Additional LED control (currently disabled)
            //vTaskDelay(pdMS_TO_TICKS(500));
            //led_set_taillight_color(LED_COLOR_RED);
        }
    }
}