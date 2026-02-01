/**
 * @file i2c_config.h
 * @brief I2C master configuration header
 * 
 * This header file defines the I2C master configuration for the Racer3 device.
 * It provides centralized I2C settings and initialization functions for
 * communication with various I2C devices such as color sensors.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "driver/i2c.h"

// I2C master configuration parameters
#define I2C_MASTER_SCL_IO           0                  // GPIO number for I2C master clock (IO0)
#define I2C_MASTER_SDA_IO           1                  // GPIO number for I2C master data (IO1)
#define I2C_MASTER_NUM              0                  // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          400000             // I2C master clock frequency (400 kHz)
#define I2C_MASTER_TX_BUF_DISABLE   0                  // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0                  // I2C master doesn't need buffer

// I2C function prototypes
/**
 * @brief Initialize I2C master peripheral
 * 
 * This function initializes the I2C master peripheral with the configured
 * parameters including GPIO pins, clock speed, and pull-up settings.
 * It sets up the I2C driver for communication with I2C devices.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t i2c_master_init(void);

#endif // I2C_CONFIG_H
