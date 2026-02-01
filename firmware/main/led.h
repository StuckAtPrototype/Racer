/**
 * @file led.h
 * @brief LED control system header
 * 
 * This header file defines the interface for the LED control system in the Racer3 device.
 * It provides functions for controlling WS2812 addressable LEDs, managing flash patterns,
 * and setting individual LED colors. The system supports various flash modes and color
 * configurations for different visual feedback scenarios.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws2812_control.h"

// Predefined LED colors in GRB format (not RGB)
#define LED_COLOR_OFF    0x000000  // Black (LEDs off)
#define LED_COLOR_RED    0x00FF00  // Red
#define LED_COLOR_GREEN  0xFF0000  // Green
#define LED_COLOR_BLUE   0x0000FF  // Blue
#define LED_COLOR_YELLOW 0xFFFF00  // Yellow
#define LED_COLOR_CYAN   0x00FFFF  // Cyan

/**
 * @brief LED flash pattern enumeration
 * 
 * Defines different flash patterns for LED behavior
 */
typedef enum {
    LED_CONST = 0,              // Constant on/off
    LED_FLASH_ALL,              // Flash all LEDs
    LED_FLASH_BACK,             // Flash only back LEDs
    LED_FLASH_FRONT,            // Flash only front LEDs
    LED_FLASH_FRONT_ALTERNATE,  // Alternate front LEDs
    LED_FLASH_INDICATOR         // Flash indicator LEDs
} led_flash;

/**
 * @brief LED configuration structure
 * 
 * Contains the current state and configuration of all LEDs
 */
typedef struct {
    led_flash mode;                    // Current flash mode
    TickType_t flash_period;           // Flash period in ticks
    bool led_state[NUM_LEDS];          // Individual LED on/off states
    uint32_t led_colors[NUM_LEDS];     // Individual LED colors (GRB format)
} led_config_t;

// Global LED configuration shared across the system
extern led_config_t led_config;

// LED system initialization and control functions
/**
 * @brief Initialize the LED control system
 * 
 * Initializes the LED driver, creates the LED task, and sets up the mutex
 * for thread-safe LED operations.
 */
void led_init(void);

/**
 * @brief Set individual LED state
 * 
 * @param led_index Index of the LED to control (0 to NUM_LEDS-1)
 * @param state True to turn on, false to turn off
 */
void set_led(int led_index, bool state);

// Color setting functions for different LED groups
/**
 * @brief Set indicator LED color
 * 
 * @param color Color value in GRB format
 */
void led_set_indicator_color(uint32_t color);

/**
 * @brief Set taillight LED color
 * 
 * @param color Color value in GRB format
 */
void led_set_taillight_color(uint32_t color);

/**
 * @brief Set headlight LED color
 * 
 * @param color Color value in GRB format
 */
void led_set_headlight_color(uint32_t color);

// Flash pattern and control functions
/**
 * @brief Set LED flash mode
 * 
 * @param mode Flash pattern to use
 */
void led_set_flash_mode(led_flash mode);

/**
 * @brief Set LED flash period
 * 
 * @param period Flash period in FreeRTOS ticks
 */
void led_set_flash_period(TickType_t period);

/**
 * @brief Turn on all LEDs
 */
void led_all_on(void);

/**
 * @brief Turn off all LEDs
 */
void led_all_off(void);

/**
 * @brief Turn on front LEDs
 */
void led_front_on(void);

/**
 * @brief Turn on back LEDs
 */
void led_back_on(void);

/**
 * @brief Set individual LED color
 * 
 * @param led_index Index of the LED to set (0 to NUM_LEDS-1)
 * @param color Color value in GRB format
 */
void led_set_individual_color(int led_index, uint32_t color);

#endif // LED_H
