/**
 * @file ws2812_control.h
 * @brief WS2812 addressable LED strip control header
 * 
 * This header file defines the interface for controlling WS2812 addressable LED strips
 * in the Racer3 device. It provides low-level control using the ESP32's RMT peripheral
 * for precise timing control required by WS2812 LEDs.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H

#include <stdint.h>
#include "sdkconfig.h"
#include "esp_err.h"

// Hardware configuration
#define NUM_LEDS	5    // Number of WS2812 LEDs in the strip

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED state structure
 * 
 * This structure defines the color state for all LEDs in the strip.
 * Each LED has a 32-bit value where only the lower 3 bytes are used:
 * - Byte 2: Red component (0-255)
 * - Byte 1: Green component (0-255)  
 * - Byte 0: Blue component (0-255)
 * 
 * Note: WS2812 LEDs expect GRB format, not RGB format.
 */
struct led_state {
    uint32_t leds[NUM_LEDS];    // Array of LED color values
};

// WS2812 control function prototypes
/**
 * @brief Initialize WS2812 control system
 * 
 * This function initializes the RMT peripheral for WS2812 control.
 * It configures the RMT channel, encoder, and timing parameters
 * required for proper WS2812 communication. Call this function
 * only once during system initialization.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ws2812_control_init(void);

/**
 * @brief Update LED strip with new colors
 * 
 * This function transmits the new LED state to the WS2812 strip.
 * It converts the color values to the proper format and uses the
 * RMT peripheral to send the data with precise timing. This function
 * blocks until the entire sequence is transmitted.
 * 
 * @param new_state The new LED state to display
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ws2812_write_leds(struct led_state new_state);

#endif

#ifdef __cplusplus
}
#endif
