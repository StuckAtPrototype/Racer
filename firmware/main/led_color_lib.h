/**
 * @file led_color_lib.h
 * @brief LED color generation and manipulation library header
 * 
 * This header file defines the interface for various color generation and
 * manipulation functions for WS2812 LEDs in the Racer3 device. It provides
 * hue-to-RGB conversion, color interpolation, pulsing effects, and full
 * spectrum color cycling capabilities.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef LED_COLOR_LIB_H
#define LED_COLOR_LIB_H

#include <stdint.h>

// Color generation constants
#define MAX_BRIGHTNESS 1.0f    // Maximum brightness (1.0 == 100%)

// Hue range definitions for temperature-based colors
#define COLOR_BLUE_HUE 17.0f   // Blue hue value for temperature range
#define COLOR_RED_HUE 27.0f    // Red hue value for temperature range

// Color generation function prototypes
/**
 * @brief Get a color based on a specific hue value
 * 
 * This function converts a 16-bit hue value to a 24-bit GRB color value
 * using the HSL color model. The hue is mapped to the full spectrum
 * (0-360 degrees) for complete color range coverage.
 * 
 * @param hue 16-bit hue value (0-65535 maps to 0-360 degrees)
 * @return 24-bit GRB color value
 */
uint32_t get_color_from_hue(uint16_t hue);

/**
 * @brief Get the next color in the full spectrum cycle
 * 
 * This function returns the next color in a continuous spectrum cycle
 * and advances the internal hue counter. It provides smooth color
 * transitions across the entire visible spectrum.
 * 
 * @return 24-bit GRB color value for the next color in the cycle
 */
uint32_t get_next_color_full_spectrum(void);

/**
 * @brief Set the hue increment for spectrum cycling
 * 
 * This function sets the step size for hue advancement in the
 * full spectrum color cycling. Larger values create faster
 * color transitions.
 * 
 * @param increment Step size for hue advancement (1-65535)
 */
void set_hue_increment(uint16_t increment);

/**
 * @brief Get interpolated color between blue and red
 * 
 * This function generates a color that is interpolated between
 * blue and red based on the input value. It's useful for
 * temperature-based color representation.
 * 
 * @param value Interpolation value (0.0 = blue, 1.0 = red)
 * @return 24-bit GRB color value
 */
uint32_t get_color_between_blue_red(float value);

/**
 * @brief Generate a pulsing color effect
 * 
 * This function creates a pulsing brightness effect using a sine wave
 * to modulate the brightness of the input RGB color. The pulsing
 * creates a breathing or heartbeat-like visual effect.
 * 
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * @return 24-bit GRB color value with pulsing brightness
 */
uint32_t get_pulsing_color(uint8_t red, uint8_t green, uint8_t blue);

#endif // LED_COLOR_LIB_H
