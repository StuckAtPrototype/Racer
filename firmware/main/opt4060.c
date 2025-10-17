/**
 * @file opt4060.c
 * @brief OPT4060 color sensor implementation
 * 
 * This file implements the OPT4060 color sensor driver for the Racer3 device.
 * It provides I2C communication, sensor initialization, and color data reading
 * capabilities. The OPT4060 is an alternative color sensor to the TCS3400.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "opt4060.h"
#include "driver/i2c.h"
#include "esp_log.h"

// Math libraries for color processing
#include <stdio.h>
#include <math.h>

static const char *TAG = "OPT4060";

/**
 * @brief Initialize I2C master for OPT4060 communication
 * 
 * This function configures and installs the I2C master driver for communication
 * with the OPT4060 color sensor.
 * 
 * @return ESP_OK on success, error code on failure
 */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,                    // Master mode
            .sda_io_num = I2C_MASTER_SDA_IO,            // SDA pin
            .scl_io_num = I2C_MASTER_SCL_IO,            // SCL pin
            .sda_pullup_en = GPIO_PULLUP_ENABLE,        // Enable SDA pullup
            .scl_pullup_en = GPIO_PULLUP_ENABLE,        // Enable SCL pullup
            .master.clk_speed = I2C_MASTER_FREQ_HZ,     // I2C clock speed
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter configuration failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "I2C driver already installed, using existing installation");
            return ESP_OK;
        }
        ESP_LOGE(TAG, "I2C driver installation failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/**
 * @brief Initialize the OPT4060 color sensor
 * 
 * This function initializes the OPT4060 color sensor by:
 * 1. Setting up I2C communication
 * 2. Configuring the sensor for continuous conversion at 1ms intervals
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t opt4060_init(void)
{
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set the chip to continuous conversion at 1ms
    // Register 0x0A: Configuration register
    // 0x0C, 0xF8: Continuous conversion mode settings
    uint8_t write_data[3] = {0x0A, 0x0C, 0xF8};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, OPT4060_SENSOR_ADDR, write_data,
                               3, pdMS_TO_TICKS(1000));

    if (ret == ESP_OK)
        ESP_LOGI(TAG, "OPT4060 initialized successfully");
    else
        ESP_LOGE(TAG, "Failed to initialize OPT4060");

    return ESP_OK;
}


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
esp_err_t opt4060_read_color(uint32_t *red, uint32_t *green, uint32_t *blue, uint32_t *clear)
{
    uint8_t data[16];  // Buffer for 16 bytes of sensor data
    esp_err_t ret;

    // Start reading from register 0
    uint8_t address[1] = {0};

    // Read 16 bytes of color data
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, OPT4060_SENSOR_ADDR, address, 1, data,
                                 16, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read color data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Convert the read data to 24-bit color values
    // Each color channel uses 3 bytes: [exponent(4 bits) + mantissa(20 bits)]
    *red =   ((((data[0] & 0xF) << 8) | data[1]) << 8) | data[2];    // Red channel
    *green = ((((data[4] & 0xF) << 8) | data[5]) << 8) | data[6];   // Green channel
    *blue =  ((((data[8] & 0xF) << 8) | data[9]) << 8) | data[10];  // Blue channel
    *clear = ((((data[12] & 0xF) << 8) | data[13]) << 8) | data[14]; // Clear channel

    // Exponent application (commented out due to saturation issues)
    // The exponent is stored in the upper 4 bits of the first byte of each channel
    // *red = (*red << (data[0] & 0xF0));
    // *green = (*green << (data[4] & 0xF0));
    // *blue = (*blue << (data[8] & 0xF0));
    // *clear = (*clear << (data[12] & 0xF0));

    return ESP_OK;
}
