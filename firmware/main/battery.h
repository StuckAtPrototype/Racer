/**
 * @file battery.h
 * @brief Battery monitoring system header
 * 
 * This header file defines the interface for the battery monitoring system
 * in the Racer3 device. It provides functions for initializing the ADC,
 * reading battery voltage, and cleaning up resources.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "esp_err.h"

/**
 * @brief Initialize the battery monitoring system
 * 
 * This function initializes the ADC peripheral for battery voltage measurement.
 * It configures the ADC channel, sets up calibration, and prepares the system
 * for voltage readings.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t battery_init(void);

/**
 * @brief Read the current battery voltage
 * 
 * This function reads the raw ADC value and converts it to a voltage reading
 * in volts. The voltage is calculated using the ADC calibration data.
 * 
 * @param voltage Pointer to store the voltage reading (in volts)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t battery_read_voltage(float *voltage);

/**
 * @brief Clean up battery monitoring resources
 * 
 * This function deinitializes the ADC peripheral and frees any allocated
 * resources used by the battery monitoring system.
 */
void battery_deinit(void);

#endif // BATTERY_H
