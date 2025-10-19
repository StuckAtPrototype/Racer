/**
 * @file tcs3400.c
 * @brief TCS3400 color sensor implementation
 * 
 * This file implements the TCS3400 color sensor driver for the Racer3 device.
 * It provides I2C communication, sensor initialization, and color data reading
 * with normalization and filtering capabilities.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "tcs3400.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdio.h>
#include <math.h>

static const char *TAG = "tcs3400";

/**
 * @brief Initialize I2C master for TCS3400 communication
 * 
 * This function configures and installs the I2C master driver for communication
 * with the TCS3400 color sensor.
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
 * @brief Initialize the TCS3400 color sensor
 * 
 * This function initializes the TCS3400 color sensor by:
 * 1. Setting up I2C communication
 * 2. Powering on the sensor and enabling ADC (0x03 to ENABLE register)
 * 3. Setting gain to 16x (0x02 to CONTROL register)
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t TCS3400_init(void)
{
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    // Turn on the module - power on and enable ADC
    // 0x30, 0x78 - write to 0x0A (ENABLE register)
    uint8_t write_data[2] = {TCS3400_REG_ENABLE, 0x03};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, TCS3400_SENSOR_ADDR, write_data,
                               2, pdMS_TO_TICKS(1000));

    // Set gain to 16x
    write_data[0] = TCS3400_REG_CONTROL;
    write_data[1] = 0x02;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, TCS3400_SENSOR_ADDR, write_data,
                                2, pdMS_TO_TICKS(1000));
                           

    if (ret == ESP_OK)
        ESP_LOGI(TAG, "TCS3400 initialized successfully");
    else
        ESP_LOGE(TAG, "Failed to initialize TCS3400");

    return ESP_OK;
}

/**
 * @brief Read color data from TCS3400 sensor
 * 
 * This function reads raw color data from the TCS3400 sensor and returns
 * normalized RGB values with high resolution (0-65535).
 * 
 * The normalization process:
 * - Raw RGB values are divided by clear channel value, giving fractions in [0,1]
 * - Fractions are scaled to 16-bit: 0..65535 (more resolution than 8-bit)
 * - Clear channel is fixed to 65535 to indicate normalization to "full scale" intensity
 * 
 * @param red Pointer to store normalized red value (0-65535)
 * @param green Pointer to store normalized green value (0-65535)
 * @param blue Pointer to store normalized blue value (0-65535)
 * @param clear Pointer to store clear channel value (always 65535 for normalization)
 * @return ESP_OK on success, error code on failure
 * 
 * @note If you need 8-bit values for logging/UI, compute them from these high-res numbers
 */
esp_err_t TCS3400_read_color(uint32_t *red, uint32_t *green, uint32_t *blue, uint32_t *clear)
{
    uint8_t data[8];  // Buffer for CDATAL/H, RDATAL/H, GDATAL/H, BDATAL/H
    esp_err_t ret;

    // Start reading from clear data low register
    uint8_t address[1] = { TCS3400_REG_CDATAL };

    // Read 8 bytes starting from clear data low register
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, TCS3400_SENSOR_ADDR,
                                       address, 1, data, 8, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read color data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Combine high and low bytes to form 16-bit raw values
    uint32_t c_raw = ((uint32_t)data[1] << 8) | data[0];  // Clear channel
    uint32_t r_raw = ((uint32_t)data[3] << 8) | data[2];  // Red channel
    uint32_t g_raw = ((uint32_t)data[5] << 8) | data[4];  // Green channel
    uint32_t b_raw = ((uint32_t)data[7] << 8) | data[6];  // Blue channel

    // Debug: Display raw RGBC values (commented out for performance)
    // ESP_LOGI("tcs3400", "Raw RGBC values - Clear: %lu, Red: %lu, Green: %lu, Blue: %lu", 
    //          (unsigned long)c_raw, (unsigned long)r_raw, (unsigned long)g_raw, (unsigned long)b_raw);

    // Reject frames where clear is too small (dark/noisy conditions)
    const uint32_t C_MIN = 10;  // Minimum clear channel threshold
    if (c_raw < C_MIN) {
        *clear = *red = *green = *blue = 0;
        ESP_LOGW("tcs3400", "Clear too low (%lu), skipping normalization.", (unsigned long)c_raw);
        return ESP_OK;
    }

    // Normalize to clear channel (fractions in [0,1]) using double for accuracy
    const double invC = 1.0 / (double)c_raw;
    const double r_frac = (double)r_raw * invC;
    const double g_frac = (double)g_raw * invC;
    const double b_frac = (double)b_raw * invC;

    // Scale to 16-bit (0..65535) and round; clamp to prevent overflow
    uint32_t r16 = (uint32_t)lrint(r_frac * 65535.0);
    uint32_t g16 = (uint32_t)lrint(g_frac * 65535.0);
    uint32_t b16 = (uint32_t)lrint(b_frac * 65535.0);

    // Clamp values to prevent overflow (should not happen with proper normalization)
    if (r16 > 65535u) r16 = 65535u;
    if (g16 > 65535u) g16 = 65535u;
    if (b16 > 65535u) b16 = 65535u;

    // Set clear channel to full scale to indicate normalization
    *clear = 65535u;
    *red   = r16;
    *green = g16;
    *blue  = b16;

    // Optional: Convert to 8-bit values for debugging/logging (derived, not returned)
    uint32_t r8 = (r16 + 257u) >> 8;  // ~round(r16/257)
    uint32_t g8 = (g16 + 257u) >> 8;
    uint32_t b8 = (b16 + 257u) >> 8;
    if (r8 > 255u) r8 = 255u;
    if (g8 > 255u) g8 = 255u;
    if (b8 > 255u) b8 = 255u;

    // Debug: Uncomment to print normalized color values for tuning or adding more colors
    // ESP_LOGI("tcs3400", "Normalized RGB (16-bit) = (%lu, %lu, %lu)  |  (8-bit) ≈ (%lu, %lu, %lu)",
    //          (unsigned long)*red, (unsigned long)*green, (unsigned long)*blue,
    //          (unsigned long)r8, (unsigned long)g8, (unsigned long)b8);

    // Debug: Uncomment to display raw data bytes for debugging
    // ESP_LOGI("tcs3400", "Raw data bytes: [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]", 
    //          data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    // Debug: Uncomment to add delay for printing values
    // vTaskDelay(10 / portTICK_PERIOD_MS);

    return ESP_OK;
}
