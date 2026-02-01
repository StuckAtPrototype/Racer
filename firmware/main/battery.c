/**
 * @file battery.c
 * @brief Battery monitoring system implementation
 * 
 * This file implements the battery monitoring system for the Racer3 device.
 * It provides ADC-based voltage measurement with optional calibration support.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

// ADC configuration for battery voltage measurement
#define ADC_UNIT ADC_UNIT_1                    // Use ADC unit 1
#define ADC_CHANNEL ADC_CHANNEL_1              // GPIO2 is ADC1_CHANNEL_0 on ESP32H2
#define ADC_ATTEN ADC_ATTEN_DB_12              // 12dB attenuation for 0-3.3V range
#define ADC_BITWIDTH ADC_BITWIDTH_DEFAULT      // Default bit width

// ADC handles and calibration state
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool do_calibration = false;

/**
 * @brief Initialize the battery monitoring system
 * 
 * This function initializes the ADC for battery voltage measurement,
 * configures the ADC channel, and sets up calibration if available.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t battery_init(void) {
    // Initialize ADC unit
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,  // Disable ULP mode for normal operation
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN,      // Set attenuation for voltage range
            .bitwidth = ADC_BITWIDTH, // Set bit width
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    // Attempt to set up ADC calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
    if (ret == ESP_OK) {
        do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
        ESP_LOGW(TAG, "ADC calibration not available, using raw values");
        do_calibration = false;
    }

    return ESP_OK;
}

/**
 * @brief Read battery voltage
 * 
 * This function reads the raw ADC value from the battery voltage measurement.
 * Currently returns raw ADC counts rather than calibrated voltage values.
 * 
 * @param voltage Pointer to store the voltage value (raw ADC counts)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t battery_read_voltage(float *voltage) {
    int raw;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC value");
        return ret;
    }

    // Return raw ADC counts (not calibrated voltage)
    // TODO: Implement voltage calibration if needed
    *voltage = (float)raw;
    ESP_LOGD(TAG, "Raw ADC counts: %d", raw);

    return ESP_OK;
}

/**
 * @brief Deinitialize the battery monitoring system
 * 
 * This function cleans up ADC resources and calibration handles.
 */
void battery_deinit(void) {
    // Clean up ADC calibration handle
    if (adc1_cali_handle) {
        adc_cali_delete_scheme_curve_fitting(adc1_cali_handle);
        adc1_cali_handle = NULL;
    }
    
    // Clean up ADC unit handle
    if (adc1_handle) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
    }
}
