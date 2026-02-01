/**
 * @file gatt_svr.h
 * @brief GATT server implementation header
 * 
 * This header file defines the interface for the GATT server implementation
 * in the Racer3 device. It provides BLE services and characteristics for
 * OTA updates, motor control, and LED color commands using the NimBLE stack.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#pragma once

#include "esp_ota_ops.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// GATT server configuration
#define LOG_TAG_GATT_SVR "gatt_svr"
#define REBOOT_DEEP_SLEEP_TIMEOUT 500

// Standard GATT service and characteristic UUIDs
#define GATT_DEVICE_INFO_UUID 0x180A        // Device Information Service
#define GATT_MANUFACTURER_NAME_UUID 0x2A29  // Manufacturer Name String
#define GATT_MODEL_NUMBER_UUID 0x2A24       // Model Number String

/**
 * @brief OTA control characteristic values enumeration
 * 
 * Defines the possible values for the OTA control characteristic
 * used for managing over-the-air firmware updates.
 */
typedef enum {
  SVR_CHR_OTA_CONTROL_NOP,           // No operation
  SVR_CHR_OTA_CONTROL_REQUEST,       // Request OTA update
  SVR_CHR_OTA_CONTROL_REQUEST_ACK,   // Acknowledge OTA request
  SVR_CHR_OTA_CONTROL_REQUEST_NAK,   // Reject OTA request
  SVR_CHR_OTA_CONTROL_DONE,          // OTA update complete
  SVR_CHR_OTA_CONTROL_DONE_ACK,      // Acknowledge OTA completion
  SVR_CHR_OTA_CONTROL_DONE_NAK,      // Reject OTA completion
} svr_chr_ota_control_val_t;


// Custom GATT service and characteristic UUIDs
/**
 * @brief OTA Service UUID
 * 
 * Custom 128-bit UUID for the Over-The-Air update service
 * UUID: d6f1d96d-594c-4c53-b1c6-244a1dfde6d8
 */
static const ble_uuid128_t gatt_svr_svc_ota_uuid =
        BLE_UUID128_INIT(0xd8, 0xe6, 0xfd, 0x1d, 0x4a, 024, 0xc6, 0xb1, 0x53, 0x4c,
                         0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6);

/**
 * @brief OTA Control Characteristic UUID
 * 
 * Custom 128-bit UUID for the OTA control characteristic
 * UUID: 7ad671aa-21c0-46a4-b722-270e3ae3d830
 */
static const ble_uuid128_t gatt_svr_chr_ota_control_uuid =
        BLE_UUID128_INIT(0x30, 0xd8, 0xe3, 0x3a, 0x0e, 0x27, 0x22, 0xb7, 0xa4, 0x46,
                         0xc0, 0x21, 0xaa, 0x71, 0xd6, 0x7a);

/**
 * @brief OTA Data Characteristic UUID
 * 
 * Custom 128-bit UUID for the OTA data characteristic
 * UUID: 23408888-1f40-4cd8-9b89-ca8d45f8a5b0
 */
static const ble_uuid128_t gatt_svr_chr_ota_data_uuid =
        BLE_UUID128_INIT(0xb0, 0xa5, 0xf8, 0x45, 0x8d, 0xca, 0x89, 0x9b, 0xd8, 0x4c,
                         0x40, 0x1f, 0x88, 0x88, 0x40, 0x23);

/**
 * @brief Color Data Characteristic UUID
 * 
 * Custom 128-bit UUID for the color data characteristic
 * UUID: 20408888-1f40-4cd8-9b89-ca8d45f8a5b0
 */
static const ble_uuid128_t gatt_svr_chr_color_data_uuid =
        BLE_UUID128_INIT(0xb0, 0xa5, 0xf8, 0x45, 0x8d, 0xca, 0x89, 0x9b, 0xd8, 0x4c,
                         0x40, 0x1f, 0x88, 0x88, 0x40, 0x20);

// GATT server function prototypes
/**
 * @brief Initialize the GATT server
 * 
 * This function initializes the GATT server with all custom services and
 * characteristics for OTA updates, motor control, and LED color commands.
 */
void gatt_svr_init();