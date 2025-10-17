/**
 * @file color_predictor.c
 * @brief Color classification and prediction system
 * 
 * This file implements a color classification system that analyzes RGB values
 * from color sensors and classifies them into predefined colors (Red, Cyan, Blue,
 * Green, Yellow, White, Unknown). It uses tolerance-based matching against
 * reference color values to determine the most likely color classification.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "color_predictor.h"
#include "esp_log.h"
#include "led.h"

// External function declaration (to be implemented in your LED control module)
extern void led_set_indicator_color(uint32_t rgb_color);

// Color names array for human-readable color identification
static const char* color_names[] = {
        "Red", "Cyan", "Blue", "Green", "Yellow", "White", "Unknown"
};

// Default color tolerance for RGB component matching
#define COLOR_TOLERANCE_DEFAULT 10000

// Individual color tolerances for RGB components
// Red color tolerances
#define COLOR_TOLERANCE_RED_R   COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_RED_G   COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_RED_B   COLOR_TOLERANCE_DEFAULT

// Cyan color tolerances
#define COLOR_TOLERANCE_CYAN_R  COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_CYAN_G  COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_CYAN_B  COLOR_TOLERANCE_DEFAULT

// Blue color tolerances
#define COLOR_TOLERANCE_BLUE_R  COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_BLUE_G  COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_BLUE_B  COLOR_TOLERANCE_DEFAULT

// Green color tolerances
#define COLOR_TOLERANCE_GREEN_R COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_GREEN_G COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_GREEN_B COLOR_TOLERANCE_DEFAULT

// Yellow color tolerances (custom values for better detection)
#define COLOR_TOLERANCE_YELLOW_R 22000
#define COLOR_TOLERANCE_YELLOW_G 19000
#define COLOR_TOLERANCE_YELLOW_B 20000

// White color tolerances
#define COLOR_TOLERANCE_WHITE_R COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_WHITE_G COLOR_TOLERANCE_DEFAULT
#define COLOR_TOLERANCE_WHITE_B COLOR_TOLERANCE_DEFAULT

// Reference RGB values for each color (16-bit normalized values)
// Red color reference values
#define COLOR_RED_R     37250
#define COLOR_RED_G     17100
#define COLOR_RED_B     15550
#define COLOR_RED_CLEAR_MIN 320000
#define COLOR_RED_CLEAR_MAX 330000

// Cyan color reference values
#define COLOR_CYAN_R    80
#define COLOR_CYAN_G    107
#define COLOR_CYAN_B    63
#define COLOR_CYAN_CLEAR_MIN 580000
#define COLOR_CYAN_CLEAR_MAX 670000

// Blue color reference values
#define COLOR_BLUE_R    12093
#define COLOR_BLUE_G    23398
#define COLOR_BLUE_B    31486
#define COLOR_BLUE_CLEAR_MIN 90136
#define COLOR_BLUE_CLEAR_MAX 92136

// Green color reference values
#define COLOR_GREEN_R   114
#define COLOR_GREEN_G   120
#define COLOR_GREEN_B   20
#define COLOR_GREEN_CLEAR_MIN 350000
#define COLOR_GREEN_CLEAR_MAX 440000

// Yellow color reference values
#define COLOR_YELLOW_R  41120
#define COLOR_YELLOW_G  52909
#define COLOR_YELLOW_B  21743
#define COLOR_YELLOW_CLEAR_MIN 670000
#define COLOR_YELLOW_CLEAR_MAX 700000

// White color reference values
#define COLOR_WHITE_R   65535
#define COLOR_WHITE_G   65535
#define COLOR_WHITE_B   56712
#define COLOR_WHITE_CLEAR_MIN 399360
#define COLOR_WHITE_CLEAR_MAX 706576

/**
 * @brief Get human-readable color name from color class
 * 
 * This function returns a string representation of the color class for
 * debugging and logging purposes.
 * 
 * @param color The color class enumeration value
 * @return String representation of the color name, or "Unknown" if invalid
 */
const char* get_color_name(color_class_t color) {
    if (color >= COLOR_RED && color <= COLOR_UNKNOWN) {
        return color_names[color];
    }
    return color_names[COLOR_UNKNOWN];
}

/**
 * @brief Classify RGB color values into predefined color categories
 * 
 * This function analyzes normalized RGB values and classifies them into one of
 * the predefined color categories using tolerance-based matching against reference
 * values. The classification is performed in order of priority (Yellow, Green, Blue,
 * Cyan, Red, White) to handle overlapping color ranges.
 * 
 * @param r Red component (0-65535)
 * @param g Green component (0-65535)
 * @param b Blue component (0-65535)
 * @param clear Clear channel value (currently unused but available for future use)
 * @return color_class_t The classified color category
 */
color_class_t classify_color_rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t clear) {

    // Yellow detection (highest priority due to overlapping ranges)
    if (r >= (COLOR_YELLOW_R - COLOR_TOLERANCE_YELLOW_R) &&
        r <= (COLOR_YELLOW_R + COLOR_TOLERANCE_YELLOW_R) &&
        g >= (COLOR_YELLOW_G - COLOR_TOLERANCE_YELLOW_G) &&
        g <= (COLOR_YELLOW_G + COLOR_TOLERANCE_YELLOW_G) &&
        b >= (COLOR_YELLOW_B - COLOR_TOLERANCE_YELLOW_B) &&
        b <= (COLOR_YELLOW_B + COLOR_TOLERANCE_YELLOW_B)) {
        return COLOR_YELLOW;
    }

    // Green detection
    if (r >= (COLOR_GREEN_R - COLOR_TOLERANCE_GREEN_R) &&
        r <= (COLOR_GREEN_R + COLOR_TOLERANCE_GREEN_R) &&
        g >= (COLOR_GREEN_G - COLOR_TOLERANCE_GREEN_G) &&
        g <= (COLOR_GREEN_G + COLOR_TOLERANCE_GREEN_G) &&
        b >= (COLOR_GREEN_B - COLOR_TOLERANCE_GREEN_B) &&
        b <= (COLOR_GREEN_B + COLOR_TOLERANCE_GREEN_B)) {
        return COLOR_GREEN;
    }

    // Blue detection
    if (r >= (COLOR_BLUE_R - COLOR_TOLERANCE_BLUE_R) &&
        r <= (COLOR_BLUE_R + COLOR_TOLERANCE_BLUE_R) &&
        g >= (COLOR_BLUE_G - COLOR_TOLERANCE_BLUE_G) &&
        g <= (COLOR_BLUE_G + COLOR_TOLERANCE_BLUE_G) &&
        b >= (COLOR_BLUE_B - COLOR_TOLERANCE_BLUE_B) &&
        b <= (COLOR_BLUE_B + COLOR_TOLERANCE_BLUE_B)) {
        return COLOR_BLUE;
    }

    // Cyan detection
    if (r >= (COLOR_CYAN_R - COLOR_TOLERANCE_CYAN_R) &&
        r <= (COLOR_CYAN_R + COLOR_TOLERANCE_CYAN_R) &&
        g >= (COLOR_CYAN_G - COLOR_TOLERANCE_CYAN_G) &&
        g <= (COLOR_CYAN_G + COLOR_TOLERANCE_CYAN_G) &&
        b >= (COLOR_CYAN_B - COLOR_TOLERANCE_CYAN_B) &&
        b <= (COLOR_CYAN_B + COLOR_TOLERANCE_CYAN_B))  {
        return COLOR_CYAN;
    }

    // Red detection
    if (r >= (COLOR_RED_R - COLOR_TOLERANCE_RED_R) &&
        r <= (COLOR_RED_R + COLOR_TOLERANCE_RED_R) &&
        g >= (COLOR_RED_G - COLOR_TOLERANCE_RED_G) &&
        g <= (COLOR_RED_G + COLOR_TOLERANCE_RED_G) &&
        b >= (COLOR_RED_B - COLOR_TOLERANCE_RED_B) &&
        b <= (COLOR_RED_B + COLOR_TOLERANCE_RED_B)) {
        return COLOR_RED;
    }

    // White detection
    if (r >= (COLOR_WHITE_R - COLOR_TOLERANCE_WHITE_R) &&
        r <= (COLOR_WHITE_R + COLOR_TOLERANCE_WHITE_R) &&
        g >= (COLOR_WHITE_G - COLOR_TOLERANCE_WHITE_G) &&
        g <= (COLOR_WHITE_G + COLOR_TOLERANCE_WHITE_G) &&
        b >= (COLOR_WHITE_B - COLOR_TOLERANCE_WHITE_B) &&
        b <= (COLOR_WHITE_B + COLOR_TOLERANCE_WHITE_B)) {
        return COLOR_WHITE;
    }

    // If we've reached here, we couldn't classify confidently
    return COLOR_UNKNOWN;
}

/**
 * @brief Process and classify color values with LED feedback
 * 
 * This function classifies the input RGB values and provides visual feedback
 * through LED indicators. Currently contains commented-out code for LED control
 * that was previously used for debugging and visual confirmation of color detection.
 * 
 * @param r Red component (0-65535)
 * @param g Green component (0-65535)
 * @param b Blue component (0-65535)
 * @param clear Clear channel value (currently unused)
 * 
 * @note The LED feedback code is commented out but preserved for future use
 */
void process_and_classify_color(uint32_t r, uint32_t g, uint32_t b, uint32_t clear) {
    // Classify the color (currently unused but available for future processing)
    color_class_t color = classify_color_rgb(r, g, b, clear);

    // Debug: Uncomment to log detected color for debugging
    // ESP_LOGD("color_predictor", "Detected color: %s (R:%u, G:%u, B:%u)",
    //          get_color_name(color), r, g, b);

    // LED feedback code (commented out but preserved for future use)
    // uint8_t led_red_byte = 0;
    // uint8_t led_green_byte = 0;
    // uint8_t led_blue_byte = 0;
    //
    // // Change LED behavior based on detected color
    // switch (color) {
    //     case COLOR_RED:
    //         led_red_byte = 255;
    //         led_green_byte = 0;
    //         led_blue_byte = 0;
    //         break;
    //     case COLOR_CYAN:
    //         led_red_byte = 0;
    //         led_green_byte = 255;
    //         led_blue_byte = 255;
    //         break;
    //     case COLOR_BLUE:
    //         led_red_byte = 0;
    //         led_green_byte = 0;
    //         led_blue_byte = 255;
    //         break;
    //     case COLOR_GREEN:
    //         led_red_byte = 0;
    //         led_green_byte = 255;
    //         led_blue_byte = 0;
    //         break;
    //     case COLOR_YELLOW:
    //         led_red_byte = 255;
    //         led_green_byte = 255;
    //         led_blue_byte = 0;
    //         break;
    //     case COLOR_WHITE:
    //         led_red_byte = 255;
    //         led_green_byte = 255;
    //         led_blue_byte = 255;
    //         break;
    //     default:
    //         break;
    // }
    //
    // uint32_t rgb_color = ((uint32_t)led_green_byte << 16) | ((uint32_t)led_red_byte << 8) | (uint32_t)led_blue_byte;
    //
    // led_set_indicator_color(rgb_color);
}