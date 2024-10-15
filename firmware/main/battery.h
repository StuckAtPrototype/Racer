#ifndef BATTERY_H
#define BATTERY_H

#include "esp_err.h"

// Initializes the ADC for battery voltage measurement
esp_err_t battery_init(void);

// Reads the battery voltage
esp_err_t battery_read_voltage(float *voltage);

#endif // BATTERY_H
