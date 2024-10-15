#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "BATTERY";

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_2  // GPIO2 is ADC1_CHANNEL_2
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_DEFAULT

static adc_oneshot_unit_handle_t adc1_handle;

esp_err_t battery_init(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    return ESP_OK;
}

esp_err_t battery_read_voltage(float *voltage) {
    int raw;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC value");
        return ret;
    }

    // Convert raw ADC value to voltage
//    *voltage = (float)raw / (float)(1 << ADC_BITWIDTH) * 3.3f; // Assuming 0-3.3V range
    *voltage = (float)raw;

    // 3300 == 2556
    // 3700 == 2865
    // 4000 == 3025

    // correct the voltage readings
    // this is due to us reading through a 10k resistor instead of direct
    // this in turn creates a small voltage divider with the resistors inside the esp body
    *voltage = (1.4925f * raw - 511.83f) / 1000.0f;

    return ESP_OK;
}
