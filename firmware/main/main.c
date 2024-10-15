// main.c

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

// BLE
#include "gap.h"
#include "gatt_svr.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "controller.h"
#include "led.h"

// battery
#include "battery.h"

// interrupt on gpio
#include "gpio_interrupt.h"

// i2c config for the color sensor
#include "i2c_config.h"

#include "opt4060.h"
#include "color_predictor.h"


#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          (15000) // Frequency in Hertz. Set frequency at 15 kHz

#define MOTOR_A_FWD_GPIO        13
#define MOTOR_A_BWD_GPIO        14
#define MOTOR_B_FWD_GPIO        4
#define MOTOR_B_BWD_GPIO        5

// Global variables
static bool motor_direction[NUM_MOTORS] = {true, true}; // true for forward, false for backward

QueueHandle_t gpio_intr_evt_queue = NULL;
NeuralNetwork nn;

void gpio_interrupt_task(void *pvParameters)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_intr_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%lu] interrupt occurred (falling edge)!\n", io_num);

            uint16_t red, green, blue, clear;
            opt4060_read_color(&red, &green, &blue, &clear);
            ESP_LOGD("main", "Color values - Red: %d, Green: %d, Blue: %d, Clear: %d, Color: White", red, green, blue, clear);

            if (gpio_get_level(INTERRUPT_PIN) == 0) {
                uint32_t color = predict_color(&nn, red, green, blue, clear);
                command_set_game_status(color);
            }
        }
    }
}

void app_main(void)
{
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

    // Initialize LEDs
    led_init();

    // set the blink rate to 100ms
    led_set_flash_period(pdMS_TO_TICKS(100));

    // Configure Motors
    configure_motor_pwm(MOTOR_A_FWD_GPIO, LEDC_CHANNEL_0);
    configure_motor_pwm(MOTOR_A_BWD_GPIO, LEDC_CHANNEL_1);
    configure_motor_pwm(MOTOR_B_FWD_GPIO, LEDC_CHANNEL_2);
    configure_motor_pwm(MOTOR_B_BWD_GPIO, LEDC_CHANNEL_3);

    // Create motor queue
    motor_queue[0] = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(MotorUpdate));
    if (motor_queue[0] == NULL) {
        printf("Failed to create motor queue\n");
        return;
    }
    motor_queue[1] = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(MotorUpdate));
    if (motor_queue[1] == NULL) {
        printf("Failed to create motor queue\n");
        return;
    }

    // Create semaphore for synchronizing motor start
    motor_start_semaphore = xSemaphoreCreateCounting(4,0);
    if (motor_start_semaphore == NULL) {
        printf("Failed to create motor start semaphore\n");
        return;
    }

    // Create motor tasks for Motor A and Motor B
    xTaskCreate(motor_task, "motor_task_A", 2048, (void *)0, 1, NULL);
    xTaskCreate(motor_task, "motor_task_B", 2048, (void *)1, 1, NULL);

    // Turn on all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        set_led(i, true);
    }

    set_led(3,false);
    set_led(2,false);
    set_led(1,true);
    set_led(0,true);


    // BLE Setup -------------------
    nimble_port_init();
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.reset_cb = reset_cb;
    gatt_svr_init();
    ble_svc_gap_device_name_set(device_name);
    nimble_port_freertos_init(host_task);

    // Initialize the motor controller
    controller_init();

    esp_err_t ret = battery_init();
    if (ret != ESP_OK) {
        ESP_LOGE("main", "Battery initialization failed");
        return;
    }


    initialize_neural_network(&nn);

    // initilize interrupt for HAL
    configure_gpio_interrupt();
    gpio_intr_evt_queue = gpio_interrupt_get_evt_queue();
    xTaskCreate(gpio_interrupt_task, "gpio_interrupt_task", 2048, NULL, 10, NULL);

    // color sensor
    opt4060_init();


    while (1) {

//        // uncomment this for training data collection
//        uint16_t red, green, blue, clear;
//        opt4060_read_color(&red, &green, &blue, &clear);
//        // NOTE "Color: White" is hardcoded -- rename this to the color you are training for
//        ESP_LOGI("main", "Color values - Red: %d, Green: %d, Blue: %d, Clear: %d, Color: Black", red, green, blue, clear);

        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay

    }
}
