/**
 * @file gap.h
 * @brief BLE GAP (Generic Access Profile) implementation header
 * 
 * This header file defines the interface for the BLE GAP functionality
 * in the Racer3 device. It provides device advertising, connection management,
 * and event handling using the NimBLE stack.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#pragma once

#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

// GAP configuration
#define LOG_TAG_GAP "gap"

// Device advertising name
static const char device_name[] = "Racer3";

// GAP function prototypes
/**
 * @brief Start BLE advertising
 * 
 * This function configures and starts BLE advertising with the device name
 * and custom service UUIDs. It sets up advertisement data and parameters
 * for device discovery and connection.
 */
void advertise();

/**
 * @brief BLE reset callback
 * 
 * This function is called when the BLE stack is reset. It handles
 * the reset event and prepares the system for reinitialization.
 * 
 * @param reason The reason for the BLE reset
 */
void reset_cb(int reason);

/**
 * @brief BLE synchronization callback
 * 
 * This function is called when the BLE stack is synchronized and ready.
 * It determines the address type and starts advertising.
 */
void sync_cb(void);

/**
 * @brief BLE host task
 * 
 * This function runs the NimBLE host stack in a FreeRTOS task.
 * It processes BLE events and maintains the BLE stack state.
 * 
 * @param param Task parameter (unused)
 */
void host_task(void *param);