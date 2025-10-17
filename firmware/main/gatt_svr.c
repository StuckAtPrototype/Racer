/**
 * @file gatt_svr.c
 * @brief GATT server implementation for BLE communication
 * 
 * This file implements the GATT server for Bluetooth Low Energy communication.
 * It handles OTA (Over-The-Air) updates, motor control commands, and color data
 * transmission between the Racer3 device and external controllers.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "gatt_svr.h"
#include "mbedtls/aes.h"
#include <string.h>
#include "controller.h"

// GATT characteristic values for BLE communication
uint8_t gatt_svr_chr_ota_control_val;      // OTA control command value
uint8_t gatt_svr_chr_ota_data_val[128];    // OTA data buffer (motor commands)
uint8_t gatt_svr_chr_color_data_val[128];  // Color data buffer (LED commands)

// GATT characteristic handles for BLE operations
uint16_t ota_control_val_handle;  // Handle for OTA control characteristic
uint16_t ota_data_val_handle;     // Handle for OTA data characteristic
uint16_t ota_color_val_handle;    // Handle for color data characteristic

// OTA communication state variables
uint16_t num_pkgs_received = 0;  // Number of packets received in current OTA session
uint16_t packet_size = 0;        // Size of packets in current OTA session

// Device information for BLE advertising
static const char *manuf_name = "StuckAtPrototype, LLC";
static const char *model_num = "Racer3";

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len);

static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg);

static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int gatt_svr_chr_color_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg);

// GATT service definitions for BLE communication
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
        {
                // Service: Device Information (Standard BLE service)
                .type = BLE_GATT_SVC_TYPE_PRIMARY,
                .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
                .characteristics =
                (struct ble_gatt_chr_def[]) {
                        {
                                // Characteristic: Manufacturer Name
                                .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
                                .access_cb = gatt_svr_chr_access_device_info,
                                .flags = BLE_GATT_CHR_F_READ,
                        },
                        {
                                // Characteristic: Model Number
                                .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
                                .access_cb = gatt_svr_chr_access_device_info,
                                .flags = BLE_GATT_CHR_F_READ,
                        },
                        {
                                0,  // End of characteristics array
                        },
                }
        },

        {
                // Service: Custom OTA Service for motor control and LED commands
                .type = BLE_GATT_SVC_TYPE_PRIMARY,
                .uuid = &gatt_svr_svc_ota_uuid.u,
                .characteristics =
                (struct ble_gatt_chr_def[]) {
                        {
                                // Characteristic: OTA Control (handles session management)
                                .uuid = &gatt_svr_chr_ota_control_uuid.u,
                                .access_cb = gatt_svr_chr_ota_control_cb,
                                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                                .val_handle = &ota_control_val_handle,
                        },
                        {
                                // Characteristic: OTA Data (motor control commands)
                                .uuid = &gatt_svr_chr_ota_data_uuid.u,
                                .access_cb = gatt_svr_chr_ota_data_cb,
                                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                                .val_handle = &ota_data_val_handle, 
                        },
                        {
                                // Characteristic: Color Data (LED control commands)
                                .uuid = &gatt_svr_chr_color_data_uuid.u,
                                .access_cb = gatt_svr_chr_color_data_cb,
                                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                                .val_handle = &ota_color_val_handle,
                        },
                        {
                                0,  // End of characteristics array
                        }
                }
        },

        {
                0,  // End of services array
        },
};

/**
 * @brief Handle access to device information characteristics
 * 
 * This function handles read requests for device information characteristics
 * including manufacturer name and model number.
 * 
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg Unused argument
 * @return BLE error code
 */
static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg) {
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    // Handle model number characteristic read
    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    // Handle manufacturer name characteristic read
    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    // Unknown characteristic UUID
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief Write data from BLE message buffer to destination buffer
 * 
 * This helper function extracts data from a BLE message buffer and writes it
 * to a destination buffer with length validation.
 * 
 * @param om Message buffer containing the data
 * @param min_len Minimum expected data length
 * @param max_len Maximum allowed data length
 * @param dst Destination buffer to write data to
 * @param len Pointer to store actual data length
 * @return BLE error code
 */
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len) {
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

/**
 * @brief Update OTA control state and send notifications
 * 
 * This function processes OTA control commands and sends appropriate
 * acknowledgements back to the client via BLE notifications.
 * 
 * @param conn_handle BLE connection handle
 */
static void update_ota_control(uint16_t conn_handle) {
    struct os_mbuf *om;
    esp_err_t err;

    // Process the received OTA control command
    switch (gatt_svr_chr_ota_control_val) {
        case SVR_CHR_OTA_CONTROL_REQUEST:
            // OTA session start request
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA session requested via BLE");

            // Acknowledge the OTA request (accept the session)
            gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_ACK;
            
            // Alternative: Reject the OTA request
            //gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_NAK;

            // Extract packet size from OTA data buffer (first 2 bytes)
            packet_size = (gatt_svr_chr_ota_data_val[1] << 8) + gatt_svr_chr_ota_data_val[0];
            ESP_LOGI(LOG_TAG_GATT_SVR, "OTA packet size: %d bytes", packet_size);

            // Reset packet counter for new OTA session
            num_pkgs_received = 0;

            // Send acknowledgement notification to client
            om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                       sizeof(gatt_svr_chr_ota_control_val));
            ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA request acknowledgement sent");

            break;

        case SVR_CHR_OTA_CONTROL_DONE:
            // OTA session completion request
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA session completion requested");

            // Perform any necessary cleanup before closing the session
            // (e.g., turn off all motors, save configuration, etc.)

            // Acknowledge the completion request
            gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_ACK;
            
            // Alternative: Reject the completion request
            //gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_NAK;

            // Send completion acknowledgement notification to client
            om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                       sizeof(gatt_svr_chr_ota_control_val));
            ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA completion acknowledgement sent");

            break;

        default:
            // Unknown or no-op control command
            break;
    }
}

/**
 * @brief Handle OTA control characteristic access
 * 
 * This function handles read and write operations on the OTA control characteristic.
 * It manages OTA session state and sends notifications to the client.
 * 
 * @param conn_handle BLE connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg Unused argument
 * @return BLE error code
 */
static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg) {
    int rc;
    uint8_t length = sizeof(gatt_svr_chr_ota_control_val);

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            // Client is reading the current OTA control value
            rc = os_mbuf_append(ctxt->om, &gatt_svr_chr_ota_control_val, length);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            // Client is writing a new OTA control value
            rc = gatt_svr_chr_write(ctxt->om, 1, length,
                                    &gatt_svr_chr_ota_control_val, NULL);

            // Process the new control value and send notifications
            update_ota_control(conn_handle);
            return rc;

        default:
            // Unsupported operation
            break;
    }

    // This should not happen with valid GATT operations
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}


/**
 * @brief Handle OTA data characteristic access (motor control commands)
 * 
 * This function processes motor control commands received via BLE.
 * The data format is: [MotorA_Speed, MotorA_Direction, MotorB_Speed, MotorB_Direction, Duration_Seconds]
 * 
 * @param conn_handle BLE connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg Unused argument
 * @return BLE error code
 */
static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg) {
    int rc;
    esp_err_t err;
    uint16_t data_len;

    // Store the received motor command data
    rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_ota_data_val),
                            gatt_svr_chr_ota_data_val, &data_len);

    // Validate we received at least 5 bytes (complete motor command)
    if (rc != 0 || data_len < 5) {
        ESP_LOGW(LOG_TAG_GATT_SVR, "Invalid OTA data length: %d (expected at least 5)", data_len);
        return rc != 0 ? rc : BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    ESP_LOGD(LOG_TAG_GATT_SVR, "Received motor command: A_speed=%d, A_dir=%d, B_speed=%d, B_dir=%d, duration=%d",
             gatt_svr_chr_ota_data_val[0], gatt_svr_chr_ota_data_val[1], 
             gatt_svr_chr_ota_data_val[2], gatt_svr_chr_ota_data_val[3],
             gatt_svr_chr_ota_data_val[4]);

    // Parse and execute motor command
    MotorCommand command = {
        .MotorASpeed = gatt_svr_chr_ota_data_val[0],      // Motor A speed (0-100%)
        .MotorADirection = gatt_svr_chr_ota_data_val[1],  // Motor A direction (0=forward, 1=backward)
        .MotorBSpeed = gatt_svr_chr_ota_data_val[2],      // Motor B speed (0-100%)
        .MotorBDirection = gatt_svr_chr_ota_data_val[3],  // Motor B direction (0=forward, 1=backward)
        .seconds = gatt_svr_chr_ota_data_val[4]           // Command duration in seconds
    };
    set_motor_command(command);

    // Update packet counter for OTA session tracking
    num_pkgs_received++;
    ESP_LOGD(LOG_TAG_GATT_SVR, "Processed motor command packet %d", num_pkgs_received);

    return rc;
}

/**
 * @brief Handle color data characteristic access (LED control commands)
 * 
 * This function processes LED color commands received via BLE.
 * The data format is: [Red, Green, Blue] (3 bytes, 0-255 each)
 * 
 * @param conn_handle BLE connection handle
 * @param attr_handle Attribute handle
 * @param ctxt GATT access context
 * @param arg Unused argument
 * @return BLE error code
 */
static int gatt_svr_chr_color_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg) {
    int rc;
    esp_err_t err;
    uint16_t data_len;

    // Clear the buffer first to prevent residual data from previous commands
    memset(gatt_svr_chr_color_data_val, 0, sizeof(gatt_svr_chr_color_data_val));
    
    // Store the received LED color data
    rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_color_data_val),
                            gatt_svr_chr_color_data_val, &data_len);

    // Validate we received exactly 3 bytes (RGB color data)
    if (rc != 0 || data_len != 3) {
        ESP_LOGW(LOG_TAG_GATT_SVR, "Invalid color data length: %d (expected 3)", data_len);
        return rc != 0 ? rc : BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    ESP_LOGD(LOG_TAG_GATT_SVR, "Received LED color command: R=%d, G=%d, B=%d",
             gatt_svr_chr_color_data_val[0], gatt_svr_chr_color_data_val[1], gatt_svr_chr_color_data_val[2]);

    // Convert RGB bytes to 32-bit color value (GRB format for WS2812 LEDs)
    uint32_t game_car_lights = ((uint32_t)gatt_svr_chr_color_data_val[1] << 16)  // Green
            | ((uint32_t)gatt_svr_chr_color_data_val[0] << 8)                    // Red
            | gatt_svr_chr_color_data_val[2];                                    // Blue

    // Apply the color to the headlight LEDs
    led_set_headlight_color(game_car_lights);

    // Note: Motor commands are handled by the OTA data characteristic
    // This characteristic is specifically for LED color control

    return rc;
}

/**
 * @brief Initialize the GATT server
 * 
 * This function initializes the GATT server with all defined services and characteristics.
 * It sets up the device information service and the custom OTA service for motor and LED control.
 */
void gatt_svr_init() {
    // Initialize GAP (Generic Access Profile) service
    ble_svc_gap_init();
    
    // Initialize GATT (Generic Attribute Profile) service
    ble_svc_gatt_init();
    
    // Configure and count the GATT services
    ble_gatts_count_cfg(gatt_svr_svcs);
    
    // Add all configured GATT services to the server
    ble_gatts_add_svcs(gatt_svr_svcs);
}
