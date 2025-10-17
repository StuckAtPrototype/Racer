/**
 * @file ws2812_control.c
 * @brief WS2812 LED strip control implementation
 * 
 * This file implements the low-level control for WS2812 addressable LED strips
 * using the ESP32's RMT (Remote Control) peripheral. It provides initialization
 * and data transmission functions for controlling individual LEDs in the strip.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "ws2812_control.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

// Hardware configuration
#define LED_RMT_TX_GPIO         25  // GPIO pin for LED data output
#define BITS_PER_LED_CMD        24  // 24 bits per LED (8 bits each for R, G, B)
#define LED_BUFFER_ITEMS        (NUM_LEDS * BITS_PER_LED_CMD)

// WS2812 timing parameters (assuming 10MHz RMT resolution)
#define T0H     3  // 0 bit high time (0.3μs)
#define T1H     6  // 1 bit high time (0.6μs)
#define T0L     8  // 0 bit low time (0.8μs)
#define T1L     5  // 1 bit low time (0.5μs)

static const char *TAG = "NeoPixel WS2812 Driver";

// Global variables for RMT control
static rmt_channel_handle_t led_chan = NULL;      // RMT channel handle
static rmt_encoder_handle_t led_encoder = NULL;   // RMT encoder handle
static uint8_t led_data_buffer[NUM_LEDS * 3];     // Buffer for LED data (3 bytes per LED: RGB)

/**
 * @brief RMT encoder wrapper function
 * 
 * This function wraps the bytes encoder for RMT transmission.
 * It's used internally by the RMT driver to encode data for transmission.
 * 
 * @param encoder RMT encoder handle
 * @param channel RMT channel handle
 * @param primary_data Data to encode
 * @param data_size Size of data to encode
 * @param ret_state Return state pointer
 * @return Number of encoded symbols
 */
static size_t ws2812_rmt_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_encoder_handle_t bytes_encoder = (rmt_encoder_handle_t)encoder;
    rmt_encode_state_t session_state = 0;
    size_t encoded_symbols = bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
    *ret_state = session_state;
    return encoded_symbols;
}

/**
 * @brief Initialize WS2812 LED control system
 * 
 * This function initializes the RMT (Remote Control) peripheral for controlling
 * WS2812 LED strips. It configures the RMT channel, sets up the timing parameters
 * for WS2812 protocol, and creates the necessary encoders.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ws2812_control_init(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
            .gpio_num = LED_RMT_TX_GPIO,           // GPIO pin for LED data
            .clk_src = RMT_CLK_SRC_DEFAULT,        // Use default clock source
            .resolution_hz = 10 * 1000 * 1000,     // 10MHz resolution for precise timing
            .mem_block_symbols = 64,               // Memory block size
            .trans_queue_depth = 4,                // Transmission queue depth
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_chan_config, &led_chan), TAG, "Failed to create RMT TX channel");

    ESP_LOGI(TAG, "Install led strip encoder");
    rmt_bytes_encoder_config_t bytes_encoder_config = {
            .bit0 = {
                    .level0 = 1,        // High level for '0' bit
                    .duration0 = T0H,   // High duration for '0' bit
                    .level1 = 0,        // Low level for '0' bit
                    .duration1 = T0L,   // Low duration for '0' bit
            },
            .bit1 = {
                    .level0 = 1,        // High level for '1' bit
                    .duration0 = T1H,   // High duration for '1' bit
                    .level1 = 0,        // Low level for '1' bit
                    .duration1 = T1L,   // Low duration for '1' bit
            },
            .flags.msb_first = 1,       // Send MSB first
    };
    ESP_RETURN_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder), TAG, "Failed to create bytes encoder");

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_RETURN_ON_ERROR(rmt_enable(led_chan), TAG, "Failed to enable RMT channel");

    return ESP_OK;
}

/**
 * @brief Write LED data to WS2812 strip
 * 
 * This function takes a structure containing LED color data and transmits it
 * to the WS2812 LED strip using the RMT peripheral. It converts the 24-bit
 * color values to the proper byte format and sends them via RMT.
 * 
 * @param new_state Structure containing color data for all LEDs
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ws2812_write_leds(struct led_state new_state)
{
    // Convert 24-bit color values to RGB byte format
    for (uint32_t led = 0; led < NUM_LEDS; led++) {
        uint32_t bits_to_send = new_state.leds[led];
        led_data_buffer[led * 3 + 0] = (bits_to_send >> 16) & 0xFF; // Red component
        led_data_buffer[led * 3 + 1] = (bits_to_send >> 8) & 0xFF;  // Green component
        led_data_buffer[led * 3 + 2] = bits_to_send & 0xFF;         // Blue component
    }

    // Configure transmission parameters
    rmt_transmit_config_t tx_config = {
            .loop_count = 0,  // No looping
    };

    // Transmit the LED data via RMT
    ESP_RETURN_ON_ERROR(rmt_transmit(led_chan, led_encoder, led_data_buffer, sizeof(led_data_buffer), &tx_config), TAG, "Failed to transmit RMT data");
    
    // Wait for transmission to complete
    ESP_RETURN_ON_ERROR(rmt_tx_wait_all_done(led_chan, portMAX_DELAY), TAG, "Failed to wait for RMT transmission to finish");

    return ESP_OK;
}
