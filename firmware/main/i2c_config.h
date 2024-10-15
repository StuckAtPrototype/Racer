#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "driver/i2c.h"

// I2C configuration
#define I2C_MASTER_SCL_IO           0                  // GPIO number for I2C master clock (IO0)
#define I2C_MASTER_SDA_IO           1                  // GPIO number for I2C master data (IO1)
#define I2C_MASTER_NUM              0                  // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          400000             // I2C master clock frequency (400 kHz)
#define I2C_MASTER_TX_BUF_DISABLE   0                  // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0                  // I2C master doesn't need buffer

// Function prototypes
esp_err_t i2c_master_init(void);

#endif // I2C_CONFIG_H
