/**
 * @file i2c_config.c
 * @brief I2C master configuration implementation
 * 
 * This file implements the I2C master configuration for the Racer3 device.
 * It provides a centralized I2C initialization function that configures
 * the I2C peripheral for communication with various sensors and devices.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "i2c_config.h"
#include "esp_log.h"

static const char *TAG = "i2c_config";

/**
 * @brief Initialize I2C master peripheral
 * 
 * This function initializes the I2C master peripheral with the configured
 * parameters including GPIO pins, clock speed, and pull-up settings.
 * It sets up the I2C driver for communication with I2C devices.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,                    // Master mode
            .sda_io_num = I2C_MASTER_SDA_IO,            // SDA GPIO pin
            .sda_pullup_en = GPIO_PULLUP_ENABLE,        // Enable SDA pull-up
            .scl_io_num = I2C_MASTER_SCL_IO,            // SCL GPIO pin
            .scl_pullup_en = GPIO_PULLUP_ENABLE,        // Enable SCL pull-up
            .master.clk_speed = I2C_MASTER_FREQ_HZ,     // I2C clock frequency
    };

    // Configure I2C parameters
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter configuration failed. Error: %s", esp_err_to_name(err));
        return err;
    }

    // Install I2C driver
    err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver installation failed. Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C master initialized successfully");
    return ESP_OK;
}
