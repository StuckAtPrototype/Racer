/**
 * @file led.c
 * @brief LED control system implementation
 * 
 * This file implements the LED control system for the Racer3 device.
 * It manages WS2812 addressable LEDs with various flash patterns,
 * color control, and thread-safe LED updates.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "led.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/semphr.h"

// LED configuration structure
led_config_t led_config = {
        .mode = LED_CONST,                    // Default mode: constant lighting
        .flash_period = pdMS_TO_TICKS(500),   // Default flash period: 500ms
        .led_state = {false, false, false, false, false},  // All LEDs initially off
        .led_colors = {0x00, 0x00, 0x00, 0x00, 0x00}       // All LEDs initially black
};

// Mutex to protect LED color updates and prevent race conditions
// between BLE headlight updates and game status indicator updates
static SemaphoreHandle_t led_mutex = NULL;

// LED layout:
// LED 0: Front left
// LED 1: Front right  
// LED 2: Back right
// LED 3: Back left
// LED 4: Indicator (center)

// Global LED color variables (GRB format for WS2812 LEDs)
uint32_t led_headlight_color = LED_COLOR_OFF;   // Front LED color
uint32_t led_taillight_color = LED_COLOR_RED;   // Rear LED color
uint32_t led_indicator_color = LED_COLOR_OFF;   // Indicator LED color

// LED state structure for WS2812 driver
struct led_state led_new_state = {0};

/**
 * @brief LED control task
 * 
 * This task manages LED patterns and updates the WS2812 LEDs based on the current
 * flash mode and color settings. It runs continuously and handles various flash
 * patterns including constant, flashing, and alternating modes.
 * 
 * @param pvParameters Task parameters (unused)
 */
static void led_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bool toggle = false;

    while (1) {
        // Take mutex to safely read LED colors
        if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Read current LED colors safely
            uint32_t headlight_color = led_headlight_color;
            uint32_t taillight_color = led_taillight_color;
            uint32_t indicator_color = led_indicator_color;
            xSemaphoreGive(led_mutex);
            
            // Apply LED pattern based on current mode
            switch (led_config.mode) {
                case LED_CONST:
                    // Constant lighting - all LEDs stay on with their assigned colors
                    led_config.led_colors[0] = headlight_color;  // Front left
                    led_config.led_colors[1] = headlight_color;  // Front right
                    led_config.led_colors[2] = taillight_color;  // Back right
                    led_config.led_colors[3] = taillight_color;  // Back left
                    led_config.led_colors[4] = indicator_color;  // Indicator
                    break;
                    
                case LED_FLASH_ALL:
                    // Flash all LEDs except indicator
                    for (int i = 0; i < NUM_LEDS-1; i++) {
                        if (toggle) {
                            if (i < 2) {
                                led_config.led_colors[i] = headlight_color;  // Front LEDs
                            } else if (i < 4) {
                                led_config.led_colors[i] = taillight_color;  // Back LEDs
                            }
                        } else {
                            led_config.led_colors[i] = 0x000000;  // Turn off
                        }
                    }
                    // Indicator LED stays constant
                    led_config.led_colors[4] = indicator_color;
                    toggle = !toggle;
                    vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                    break;
                    
                case LED_FLASH_BACK:
                    // Flash only rear LEDs
                    led_config.led_colors[0] = headlight_color;  // Front LEDs stay on
                    led_config.led_colors[1] = headlight_color;
                    if (toggle) {
                        led_config.led_colors[2] = taillight_color;  // Back LEDs on
                        led_config.led_colors[3] = taillight_color;
                    } else {
                        led_config.led_colors[2] = 0x000000;  // Back LEDs off
                        led_config.led_colors[3] = 0x000000;
                    }
                    led_config.led_colors[4] = indicator_color;  // Indicator stays on
                    toggle = !toggle;
                    vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                    break;
                    
                case LED_FLASH_FRONT:
                    // Flash only front LEDs
                    if (toggle) {
                        led_config.led_colors[0] = headlight_color;  // Front LEDs on
                        led_config.led_colors[1] = headlight_color;
                    } else {
                        led_config.led_colors[0] = 0x000000;  // Front LEDs off
                        led_config.led_colors[1] = 0x000000;
                    }
                    led_config.led_colors[2] = taillight_color;  // Back LEDs stay on
                    led_config.led_colors[3] = taillight_color;
                    led_config.led_colors[4] = indicator_color;  // Indicator stays on
                    toggle = !toggle;
                    vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                    break;
                    
                case LED_FLASH_FRONT_ALTERNATE:
                    // Alternate front LEDs
                    if (toggle) {
                        led_config.led_colors[0] = headlight_color;  // Left front on
                        led_config.led_colors[1] = 0x000000;         // Right front off
                    } else {
                        led_config.led_colors[0] = 0x000000;         // Left front off
                        led_config.led_colors[1] = headlight_color;  // Right front on
                    }
                    led_config.led_colors[2] = taillight_color;  // Back LEDs stay on
                    led_config.led_colors[3] = taillight_color;
                    led_config.led_colors[4] = indicator_color;  // Indicator stays on
                    toggle = !toggle;
                    vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                    break;
                    
                case LED_FLASH_INDICATOR:
                    // Flash only indicator LED
                    led_config.led_colors[0] = headlight_color;  // Front LEDs stay on
                    led_config.led_colors[1] = headlight_color;
                    led_config.led_colors[2] = taillight_color;  // Back LEDs stay on
                    led_config.led_colors[3] = taillight_color;
                    if (toggle) {
                        led_config.led_colors[4] = indicator_color;  // Indicator on
                    } else {
                        led_config.led_colors[4] = 0x000000;         // Indicator off
                    }
                    toggle = !toggle;
                    vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                    break;
            }

            // Update WS2812 LEDs with new color configuration
            for(int i = 0; i < NUM_LEDS; i++){
                led_new_state.leds[i] = led_config.led_colors[i];
            }
            ws2812_write_leds(led_new_state);
        } else {
            // Fallback mode if mutex is not available
            if (led_mutex == NULL) {
                // Mutex not available, use direct access (not thread-safe but won't crash)
                ESP_LOGW("led", "LED mutex not available, using direct access");
                led_config.led_colors[0] = led_headlight_color;
                led_config.led_colors[1] = led_headlight_color;
                led_config.led_colors[2] = led_taillight_color;
                led_config.led_colors[3] = led_taillight_color;
                led_config.led_colors[4] = led_indicator_color;
                
                // Update WS2812 LEDs
                for(int i = 0; i < NUM_LEDS; i++){
                    led_new_state.leds[i] = led_config.led_colors[i];
                }
                ws2812_write_leds(led_new_state);
            } else {
                ESP_LOGW("led", "Failed to take LED mutex - skipping update");
            }
        }

        // Task delay for 100ms (10Hz update rate)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Initialize the LED control system
 * 
 * This function initializes the WS2812 LED driver, creates the LED mutex
 * for thread-safe operations, and starts the LED control task.
 */
void led_init(void) {
    // Initialize WS2812 LED driver
    ws2812_control_init();

    // Create mutex for thread-safe LED color updates
    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) {
        ESP_LOGE("led", "Failed to create LED mutex - system will be unstable");
        // Continue execution but log the error - system will use fallback mode
    }

    // Create LED control task with increased stack size
    BaseType_t ret = xTaskCreate(led_task, "led_task", 4096, NULL, 10, NULL);
    if (ret != pdPASS) {
        ESP_LOGE("led", "Failed to create LED task");
    }
}


/**
 * @brief Set indicator LED color
 * 
 * This function sets the color of the indicator LED (LED 4) in a thread-safe manner.
 * 
 * @param color Color value in GRB format (0x00RRGGBB)
 */
void led_set_indicator_color(uint32_t color){
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        led_indicator_color = color;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        // Fallback: direct assignment if mutex not available
        led_indicator_color = color;
    }
}

/**
 * @brief Set taillight LED color
 * 
 * This function sets the color of the taillight LEDs (rear LEDs) in a thread-safe manner.
 * 
 * @param color Color value in GRB format (0x00RRGGBB)
 */
void led_set_taillight_color(uint32_t color){
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        led_taillight_color = color;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        // Fallback: direct assignment if mutex not available
        led_taillight_color = color;
    }
}

/**
 * @brief Set headlight LED color
 * 
 * This function sets the color of the headlight LEDs (front LEDs) in a thread-safe manner.
 * 
 * @param color Color value in GRB format (0x00RRGGBB)
 */
void led_set_headlight_color(uint32_t color){
    if (led_mutex != NULL && xSemaphoreTake(led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        led_headlight_color = color;
        xSemaphoreGive(led_mutex);
    } else if (led_mutex == NULL) {
        // Fallback: direct assignment if mutex not available
        led_headlight_color = color;
    }
}

/**
 * @brief Set LED flash mode
 * 
 * This function sets the LED flash pattern mode.
 * 
 * @param mode LED flash mode (LED_CONST, LED_FLASH_ALL, etc.)
 */
void led_set_flash_mode(led_flash mode)
{
    led_config.mode = mode;
    ESP_LOGD("led", "LED flash mode set to %d", mode);
}

/**
 * @brief Set LED flash period
 * 
 * This function sets the flash period for LED patterns.
 * 
 * @param period Flash period in FreeRTOS ticks
 */
void led_set_flash_period(TickType_t period)
{
    led_config.flash_period = period;
    ESP_LOGD("led", "LED flash period set to %u ticks", (unsigned int)period);
}

/**
 * @brief Turn on front LEDs (constant mode)
 * 
 * This function sets the LED mode to constant lighting.
 */
void led_front_on(void)
{
    led_set_flash_mode(LED_CONST);
}

/**
 * @brief Turn off front LEDs
 * 
 * This function sets the LED mode to off (not implemented - would need LED_COLOR_OFF mode).
 */
void led_front_off(void)
{
    led_set_flash_mode(LED_COLOR_OFF);  // Note: This should probably be LED_CONST with off colors
}

/**
 * @brief Turn on back LEDs (constant mode)
 * 
 * This function sets the LED mode to constant lighting.
 */
void led_back_on(void)
{
    led_set_flash_mode(LED_CONST);
}

/**
 * @brief Set individual LED color
 * 
 * This function sets the color of a specific LED by index.
 * 
 * @param led_index LED index (0-4)
 * @param color Color value in GRB format (0x00RRGGBB)
 */
void led_set_individual_color(int led_index, uint32_t color)
{
    if (led_index >= 0 && led_index < NUM_LEDS) {
        led_config.led_colors[led_index] = color;
    }
}