#include "opt4060.h"
#include "driver/i2c.h"
#include "esp_log.h"

// math
#include <stdio.h>
#include <math.h>

static const char *TAG = "OPT4060";

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter configuration failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "I2C driver already installed, attempting to use existing installation");
            return ESP_OK;
        }
        ESP_LOGE(TAG, "I2C driver installation failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t opt4060_init(void)
{
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    // set the chip to continuous conversion at 1ms
    // 0x30, 0x78 - write to 0x0A
    uint8_t write_data[3] = {0x0A, 0x30, 0x78};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, OPT4060_SENSOR_ADDR, write_data,
                               3, pdMS_TO_TICKS(1000));

    if (ret == ESP_OK)
        ESP_LOGI(TAG, "OPT4060 initialized successfully");
    else
        ESP_LOGE(TAG, "failed to init OPT4060");

    return ESP_OK;
}

void determineColor(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
//    // Normalize the values
//    float r = (float)red / clear;
//    float g = (float)green / clear;
//    float b = (float)blue / clear;
//
//    // Define color thresholds
//    float threshold = 0.2;
//    float white_threshold = 0.9;
//    float black_threshold = 0.1;
//
//    // Determine the color
//    if (r < black_threshold && g < black_threshold && b < black_threshold) {
//        printf("Black\n");
//    } else if (r > white_threshold && g > white_threshold && b > white_threshold) {
//        printf("White\n");
//    } else if (r > threshold && r > g && r > b) {
//        printf("Red\n");
//    } else if (g > threshold && g > r && g > b) {
//        printf("Green\n");
//    } else if (b > threshold && b > r && b > g) {
//        printf("Blue\n");
//    } else if (r > threshold && g > threshold && r > b && g > b) {
//        printf("Yellow\n");
//    } else if (r > threshold && b > threshold && r > g && b > g) {
//        printf("Magenta\n");
//    } else if (g > threshold && b > threshold && g > r && b > r) {
//        printf("Cyan\n");
//    } else {
//        printf("Unknown\n");
//    }

    // Define thresholds based on the provided examples
    const uint16_t GREEN_THRESHOLD = 1000;
    const uint16_t BLACK_THRESHOLD = 300;
    const uint16_t WHITE_THRESHOLD = 1000;
    const uint16_t CLEAR_THRESHOLD = 3000;

    // Calculate ratios for more accurate color detection
    float red_ratio = (float)red / (red + green + blue);
    float green_ratio = (float)green / (red + green + blue);
    float blue_ratio = (float)blue / (red + green + blue);

    // Check for green
    if (green > GREEN_THRESHOLD && green_ratio > 0.4) {
        ESP_LOGW("opt4060", "Green");
    }
        // Check for black
    else if (red < BLACK_THRESHOLD && green < BLACK_THRESHOLD && blue < BLACK_THRESHOLD) {
        ESP_LOGW("opt4060", "Black");
    }
        // Check for white
    else if (red > WHITE_THRESHOLD && green > WHITE_THRESHOLD && blue > WHITE_THRESHOLD && clear > CLEAR_THRESHOLD) {
        ESP_LOGW("opt4060", "White");
    }
        // Check for red
    else if (red_ratio > 0.35 && red > green && red > blue) {
        ESP_LOGW("opt4060", "Red");
    }
        // Check for blue
    else if (blue_ratio > 0.35 && blue > red && blue > green) {
        ESP_LOGW("opt4060", "Blue");
    }
        // If no specific color is detected
    else {
        ESP_LOGW("opt4060", "Unknown");
    }

}

void classify_color(double X, double Y, double Z, double LUX) {
    // Convert XYZ to RGB
    double R = 3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z;
    double G = -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z;
    double B = 0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z;

    // Normalize RGB values
    R = fmax(0, fmin(1, R));
    G = fmax(0, fmin(1, G));
    B = fmax(0, fmin(1, B));

    // Define thresholds
    const double COLOR_THRESHOLD = 0.5;
    const double BLACK_THRESHOLD = 0.1;
    const double WHITE_THRESHOLD = 0.9;

    // Check for black and white first
    if (LUX < BLACK_THRESHOLD) {
        ESP_LOGW(TAG, "Classified color: Black");
    } else if (LUX > WHITE_THRESHOLD && R > WHITE_THRESHOLD && G > WHITE_THRESHOLD && B > WHITE_THRESHOLD) {
        ESP_LOGW(TAG, "Classified color: White");
    }
        // Classify based on dominant color
    else if (R > COLOR_THRESHOLD && R > G && R > B) {
        ESP_LOGW(TAG, "Classified color: Red");
    } else if (G > COLOR_THRESHOLD && G > R && G > B) {
        ESP_LOGW(TAG, "Classified color: Green");
    } else if (B > COLOR_THRESHOLD && B > R && B > G) {
        ESP_LOGW(TAG, "Classified color: Blue");
    }
        // Default case
    else {
        ESP_LOGW(TAG, "Classified color: Unclassified");
    }
}

esp_err_t opt4060_read_color(uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear)
{

    uint8_t data[16];
    esp_err_t ret;

    uint8_t address[1] = {0};

    ret = i2c_master_write_read_device(I2C_MASTER_NUM, OPT4060_SENSOR_ADDR, address, 1, data,
                                 16, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read color data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Convert the read data to color values
//    *red = (data[1] << 8) | data[0];
//    *green = (data[3] << 8) | data[2];
//    *blue = (data[5] << 8) | data[4];
//    *clear = (data[7] << 8) | data[6];

    *red =  ((data[0] & 0xF) << 8) | data[1];
    *green = ((data[4] & 0xF) << 8) | data[5];
    *blue =  ((data[8] & 0xF) << 8) | data[9];
    *clear =  ((data[12] & 0xF) << 8) | data[13];





    return ESP_OK;
}
