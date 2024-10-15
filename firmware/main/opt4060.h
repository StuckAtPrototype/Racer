#ifndef OPT4060_H
#define OPT4060_H

#include <stdint.h>
#include "esp_err.h"

#define I2C_MASTER_SCL_IO           0      // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           1      // GPIO number for I2C master data
#define I2C_MASTER_NUM              0      // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          100000 // I2C master clock frequency
#define OPT4060_SENSOR_ADDR         0x44   // OPT4060 I2C address (1000100 in binary)

#define OPT4060_REG_COLOR           0x00   // Register address for color data

esp_err_t opt4060_init(void);
void determineColor(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear);
void classify_color(double X, double Y, double Z, double LUX);
esp_err_t opt4060_read_color(uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear);

#endif // OPT4060_H
