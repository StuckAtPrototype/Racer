/**
 * @file color_predictor.h
 * @brief Color classification and prediction system header
 * 
 * This header file defines the interface for the color classification system
 * in the Racer3 device. It provides functions for analyzing RGB values from
 * color sensors and classifying them into predefined colors using tolerance-based
 * matching against reference color values.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef COLOR_PREDICTOR_H
#define COLOR_PREDICTOR_H

#include <stdint.h>

/**
 * @brief Color classification enumeration
 * 
 * Defines the possible color categories that can be detected by the system.
 * Colors are classified using tolerance-based matching against reference values.
 */
typedef enum {
    COLOR_RED,      // Red color
    COLOR_CYAN,     // Cyan color
    COLOR_BLUE,     // Blue color
    COLOR_GREEN,    // Green color
    COLOR_YELLOW,   // Yellow color
    COLOR_WHITE,    // White color
    COLOR_UNKNOWN   // Unknown/unclassified color
} color_class_t;

/**
 * @brief Classify a color based on RGB and clear values
 * 
 * This function analyzes normalized RGB values and classifies them into one of
 * the predefined color categories using tolerance-based matching. The classification
 * is performed in order of priority to handle overlapping color ranges.
 *
 * @param r Red component (0-65535, normalized 16-bit value)
 * @param g Green component (0-65535, normalized 16-bit value)
 * @param b Blue component (0-65535, normalized 16-bit value)
 * @param clear Clear channel value (currently unused but available for future use)
 * @return color_class_t The classified color category
 */
color_class_t classify_color_rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t clear);

/**
 * @brief Process RGB and clear values and classify the color
 * 
 * This function classifies the input RGB values and provides visual feedback
 * through LED indicators. Currently contains commented-out code for LED control
 * that was previously used for debugging and visual confirmation of color detection.
 *
 * @param r Red component (0-65535, normalized 16-bit value)
 * @param g Green component (0-65535, normalized 16-bit value)
 * @param b Blue component (0-65535, normalized 16-bit value)
 * @param clear Clear channel value (currently unused)
 */
void process_and_classify_color(uint32_t r, uint32_t g, uint32_t b, uint32_t clear);

/**
 * @brief Get the string name of a color class
 * 
 * Returns a human-readable string representation of the color class
 * for debugging and logging purposes.
 *
 * @param color The color class to get the name of
 * @return const char* String representation of the color name
 */
const char* get_color_name(color_class_t color);

#endif /* COLOR_PREDICTOR_H */