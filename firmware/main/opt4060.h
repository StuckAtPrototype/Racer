/**
 * @file opt4060.h
 * @brief OPT4060 color sensor driver header
 * 
 * This header file defines the interface for the OPT4060 color sensor driver
 * in the Racer3 device. It provides I2C communication, sensor initialization,
 * and color data reading capabilities. The OPT4060 is an alternative color
 * sensor to the TCS3400.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef OPT4060_H
#define OPT4060_H

#include <stdint.h>
#include "esp_err.h"

// I2C configuration for OPT4060 communication
#define I2C_MASTER_SCL_IO           0      // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           1      // GPIO number for I2C master data
#define I2C_MASTER_NUM              0      // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          100000 // I2C master clock frequency
#define OPT4060_SENSOR_ADDR         0x44   // OPT4060 I2C address (1000100 in binary)

// OPT4060 register addresses
#define OPT4060_REG_COLOR           0x00   // Register address for color data

// OPT4060 function prototypes
/**
 * @brief Initialize the OPT4060 color sensor
 * 
 * This function initializes the OPT4060 color sensor by setting up I2C communication
 * and configuring the sensor for continuous conversion at 1ms intervals.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t opt4060_init(void);

/**
 * @brief Read color data from OPT4060 sensor
 * 
 * This function reads raw color data from the OPT4060 sensor and converts
 * it to 24-bit color values. The sensor provides 16 bytes of data containing
 * red, green, blue, and clear channel values with exponent information.
 * 
 * @param red Pointer to store red component value
 * @param green Pointer to store green component value
 * @param blue Pointer to store blue component value
 * @param clear Pointer to store clear channel value
 * @return ESP_OK on success, error code on failure
 */
esp_err_t opt4060_read_color(uint32_t *red, uint32_t *green, uint32_t *blue, uint32_t *clear);

// Legacy function prototypes (commented out for reference)
// void determineColor(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear);
// void classify_color(double X, double Y, double Z, double LUX);

#endif // OPT4060_H
