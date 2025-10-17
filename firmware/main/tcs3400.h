/**
 * @file tcs3400.h
 * @brief TCS3400 color sensor driver header
 * 
 * This header file defines the interface for the TCS3400 color sensor driver
 * in the Racer3 device. It provides I2C communication, sensor initialization,
 * and color data reading capabilities with normalization and filtering.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef TCS3400_H
#define TCS3400_H

#include <stdint.h>
#include "esp_err.h"

// I2C configuration for TCS3400 communication
#define I2C_MASTER_SCL_IO           0      // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           1      // GPIO number for I2C master data
#define I2C_MASTER_NUM              0      // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ          100000 // I2C master clock frequency
#define TCS3400_SENSOR_ADDR         0x39   // TCS34001FNM I2C address 

/**
 * @brief TCS3400 register map documentation
 * 
 * The following registers are available in the TCS3400 color sensor:
 * - 0x80 ENABLE: Controls power on/off, ADC enable, interrupt enable, wait timer, and sleep after interrupt
 * - 0x81 ATIME: RGBC integration time setting (2.78 ms default -- keep it)
 * - 0x83 WTIME: Wait time setting
 * - 0x84-0x87 AILTL, AILTH, AIHTL, AIHTH: Interrupt low and high threshold registers for clear channel
 * - 0x8C PERS: Interrupt persistence filter
 * - 0x8D CONFIG: Configuration register
 * - 0x8F CONTROL: Gain control register
 * - 0x90 AUX: Auxiliary control register
 * - 0x91 REVID: Revision ID (read-only)
 * - 0x92 ID: Device ID (read-only)
 * - 0x93 STATUS: Device status (read-only)
 * - 0x94-0x9B CDATAL to BDATAH: 16-bit clear, red, green, and blue ADC data registers (low and high bytes)
 * - 0xC0 IR: IR channel access register
 * - 0xE4 IFORCE: Force interrupt register (write-only)
 * - 0xE6 CICLEAR: Clear channel interrupt clear register (write-only)
 * - 0xE7 AICLEAR: Clear all interrupts register (write-only)
 */

// TCS3400 register addresses
#define TCS3400_REG_ENABLE          0x80   // Enable register
#define TCS3400_REG_ATIME           0x81   // Integration time register
#define TCS3400_REG_WTIME           0x83   // Wait time register
#define TCS3400_REG_AILTL           0x84   // Interrupt low threshold low byte
#define TCS3400_REG_AILTH           0x85   // Interrupt low threshold high byte
#define TCS3400_REG_AIHTL           0x86   // Interrupt high threshold low byte
#define TCS3400_REG_AIHTH           0x87   // Interrupt high threshold high byte
#define TCS3400_REG_PERS            0x8C   // Interrupt persistence filter
#define TCS3400_REG_CONFIG          0x8D   // Configuration register
#define TCS3400_REG_CONTROL         0x8F   // Gain control register
#define TCS3400_REG_AUX             0x90   // Auxiliary control register
#define TCS3400_REG_REVID           0x91   // Revision ID register
#define TCS3400_REG_ID              0x92   // Device ID register
#define TCS3400_REG_STATUS          0x93   // Status register
#define TCS3400_REG_CDATAL          0x94   // Clear data low byte
#define TCS3400_REG_CDATAH          0x95   // Clear data high byte
#define TCS3400_REG_RDATAL          0x96   // Red data low byte
#define TCS3400_REG_RDATAH          0x97   // Red data high byte
#define TCS3400_REG_GDATAL          0x98   // Green data low byte
#define TCS3400_REG_GDATAH          0x99   // Green data high byte
#define TCS3400_REG_BDATAL          0x9A   // Blue data low byte
#define TCS3400_REG_BDATAH          0x9B   // Blue data high byte
#define TCS3400_REG_IR              0xC0   // IR channel access register
#define TCS3400_REG_IFORCE          0xE4   // Force interrupt register
#define TCS3400_REG_CICLEAR         0xE6   // Clear channel interrupt clear
#define TCS3400_REG_AICLEAR         0xE7   // Clear all interrupts register




// TCS3400 function prototypes
/**
 * @brief Initialize the TCS3400 color sensor
 * 
 * This function initializes the TCS3400 color sensor by setting up I2C communication,
 * powering on the sensor, enabling ADC, and configuring integration time and gain.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t TCS3400_init(void);

/**
 * @brief Read color data from TCS3400 sensor
 * 
 * This function reads raw color data from the TCS3400 sensor and returns
 * normalized RGB values with high resolution (0-65535). The normalization
 * process divides raw RGB values by the clear channel value for consistent
 * color representation regardless of ambient light conditions.
 * 
 * @param red Pointer to store normalized red value (0-65535)
 * @param green Pointer to store normalized green value (0-65535)
 * @param blue Pointer to store normalized blue value (0-65535)
 * @param clear Pointer to store clear channel value (always 65535 for normalization)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t TCS3400_read_color(uint32_t *red, uint32_t *green, uint32_t *blue, uint32_t *clear);

// Legacy function prototypes (commented out for reference)
// void determineColor(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear);
// void classify_color(double X, double Y, double Z, double LUX);

#endif // TCS3400_H
