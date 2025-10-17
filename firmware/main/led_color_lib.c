/**
 * @file led_color_lib.c
 * @brief LED color library implementation
 * 
 * This file implements various color generation and manipulation functions for the Racer3 device.
 * It provides hue-to-RGB conversion, color interpolation, pulsing effects, and full spectrum
 * color cycling capabilities for WS2812 LED control.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "led_color_lib.h"
#include <math.h>

// Static variables for color cycling and effects
static uint16_t hue_increment = 10;  // Increment for hue cycling
static uint16_t current_hue = 0;     // Current hue position

/**
 * @brief Convert hue to RGB values
 * 
 * This helper function converts a hue value (0.0 to 1.0) to RGB values using
 * the HSL color space model. It implements the standard hue-to-RGB conversion
 * algorithm for the six color sectors.
 * 
 * @param h Hue value (0.0 to 1.0, where 0=red, 1/6=yellow, 2/6=green, etc.)
 * @param r Pointer to store red component (0.0 to 1.0)
 * @param g Pointer to store green component (0.0 to 1.0)
 * @param b Pointer to store blue component (0.0 to 1.0)
 */
static void hue_to_rgb(float h, float *r, float *g, float *b) {
    // Calculate the intermediate value for color interpolation
    float x = 1 - fabsf(fmodf(h * 6, 2) - 1);

    // Convert hue to RGB based on the six color sectors
    if (h < 1.0f/6.0f)      { *r = 1; *g = x; *b = 0; }  // Red to Yellow
    else if (h < 2.0f/6.0f) { *r = x; *g = 1; *b = 0; }  // Yellow to Green
    else if (h < 3.0f/6.0f) { *r = 0; *g = 1; *b = x; }  // Green to Cyan
    else if (h < 4.0f/6.0f) { *r = 0; *g = x; *b = 1; }  // Cyan to Blue
    else if (h < 5.0f/6.0f) { *r = x; *g = 0; *b = 1; }  // Blue to Magenta
    else                    { *r = 1; *g = 0; *b = x; }  // Magenta to Red
}

/**
 * @brief Get RGB color from hue value
 * 
 * This function converts a 16-bit hue value to a 24-bit RGB color value
 * in GRB format suitable for WS2812 LEDs.
 * 
 * @param hue Hue value (0-65535, where 0=red, 10923=yellow, 21845=green, etc.)
 * @return 24-bit color value in GRB format
 */
uint32_t get_color_from_hue(uint16_t hue) {
    // Convert 16-bit hue to 0.0-1.0 range
    float h = hue / 65536.0f;
    float r, g, b;

    // Convert hue to RGB
    hue_to_rgb(h, &r, &g, &b);

    // Apply brightness limit and scale to 0-255 range
    r *= MAX_BRIGHTNESS * 255;
    g *= MAX_BRIGHTNESS * 255;
    b *= MAX_BRIGHTNESS * 255;

    // Convert to GRB format for WS2812 LEDs
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

/**
 * @brief Get next color in full spectrum cycle
 * 
 * This function returns the next color in a continuous spectrum cycle.
 * Each call advances the hue by the configured increment amount.
 * 
 * @return 24-bit color value in GRB format
 */
uint32_t get_next_color_full_spectrum(void) {
    uint32_t color = get_color_from_hue(current_hue);

    // Increment hue for next call
    current_hue += hue_increment;
    if (current_hue >= 65536) current_hue = 0;  // Wrap around

    return color;
}

/**
 * @brief Set the hue increment for color cycling
 * 
 * This function sets the step size for hue advancement in the full spectrum cycle.
 * Larger values result in faster color transitions.
 * 
 * @param increment Hue increment value (0-65535)
 */
void set_hue_increment(uint16_t increment) {
    hue_increment = increment;
}


/**
 * @brief Get color interpolated between blue and red
 * 
 * This function creates a color that interpolates between blue and red based on
 * the input value. It's useful for creating color gradients that represent
 * different states or values.
 * 
 * @param value Input value (should be between COLOR_BLUE_HUE and COLOR_RED_HUE)
 * @return 24-bit color value in GRB format
 */
uint32_t get_color_between_blue_red(float value) {
    float ratio;
    float r, g, b;

    // Ensure value is within the valid range
    if (value < COLOR_BLUE_HUE) value = COLOR_BLUE_HUE;
    if (value > COLOR_RED_HUE) value = COLOR_RED_HUE;

    // Calculate the ratio (0.0 for BLUE, 1.0 for RED)
    ratio = (value - COLOR_BLUE_HUE) / (COLOR_RED_HUE - COLOR_BLUE_HUE);

    // Interpolate between BLUE (0, 0, 1) and RED (1, 0, 0)
    r = ratio;
    g = 0.0f;
    b = 1.0f - ratio;

    // Apply brightness limit and scale to 0-255 range
    r *= MAX_BRIGHTNESS * 255;
    g *= MAX_BRIGHTNESS * 255;
    b *= MAX_BRIGHTNESS * 255;

    // Convert to GRB format for WS2812 LEDs
    return ((uint32_t)(g + 0.5f) << 16) | ((uint32_t)(r + 0.5f) << 8) | (uint32_t)(b + 0.5f);
}

// Static variables for pulsing effect
static uint32_t pulse_time_ms = 0;  // Current pulse time in milliseconds
#define PULSE_MS 100  // Pulse period in milliseconds

/**
 * @brief Get pulsing color effect
 * 
 * This function creates a pulsing effect by modulating the brightness of a given
 * color using a sine wave. The color pulses smoothly from dim to bright and back.
 * 
 * @param red Red component of the base color (0-255)
 * @param green Green component of the base color (0-255)
 * @param blue Blue component of the base color (0-255)
 * @return 24-bit color value in GRB format with pulsing brightness
 */
uint32_t get_pulsing_color(uint8_t red, uint8_t green, uint8_t blue) {
    // Calculate the phase of the pulse (0 to 2π)
    float phase = (pulse_time_ms / (float)PULSE_MS) * 2 * M_PI;

    // Use a sine wave to create a smooth pulse (range: 0 to 0.5)
    float brightness = (sinf(phase) + 1) / 4;

    // Apply the brightness to the specified color
    float r = brightness * red;
    float g = brightness * green;
    float b = brightness * blue;

    // Increment the time (function is called every 10ms)
    pulse_time_ms += 1;
    if (pulse_time_ms >= PULSE_MS) {
        pulse_time_ms = 0;  // Reset after PULSE_MS milliseconds
    }

    // Convert to GRB format for WS2812 LEDs
    return ((uint32_t)(g + 0.5f) << 16) | ((uint32_t)(r + 0.5f) << 8) | (uint32_t)(b + 0.5f);
}