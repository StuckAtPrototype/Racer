#include "gatt_svr.h"
#include "mbedtls/aes.h"
#include <string.h>
#include "controller.h"


uint8_t gatt_svr_chr_ota_control_val;
uint8_t gatt_svr_chr_ota_data_val[128];

uint16_t ota_control_val_handle;
uint16_t ota_data_val_handle;

uint16_t num_pkgs_received = 0;
uint16_t packet_size = 0;

static const char *manuf_name = "DreamNight LLC";
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

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
        {// Service: Device Information
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
                                0,
                        },
                }},

        {
                // service: OTA Service
                .type = BLE_GATT_SVC_TYPE_PRIMARY,
                .uuid = &gatt_svr_svc_ota_uuid.u,
                .characteristics =
                (struct ble_gatt_chr_def[]) {
                        {
                                // characteristic: OTA control
                                .uuid = &gatt_svr_chr_ota_control_uuid.u,
                                .access_cb = gatt_svr_chr_ota_control_cb,
                                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                                         BLE_GATT_CHR_F_NOTIFY,
                                .val_handle = &ota_control_val_handle,
                        },
                        {
                                // characteristic: OTA data
                                .uuid = &gatt_svr_chr_ota_data_uuid.u,
                                .access_cb = gatt_svr_chr_ota_data_cb,
                                .flags = BLE_GATT_CHR_F_WRITE,
                                .val_handle = &ota_data_val_handle,
                        },
                        {
                                0,
                        }},
        },

        {
                0,
        },
};

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg) {
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

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

static void update_ota_control(uint16_t conn_handle) {
    struct os_mbuf *om;
    esp_err_t err;

    // check which value has been received
    switch (gatt_svr_chr_ota_control_val) {
        case SVR_CHR_OTA_CONTROL_REQUEST:
            // OTA request
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA has been requested via BLE.");

            // is this only the initial request, or does this happen on every transaction?

            // this is saying back to the other device that we are good to go
            gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_ACK;

            // this would say that we are not good to go
//      gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_NAK;

            // retrieve the packet size from OTA data
            packet_size =
                    (gatt_svr_chr_ota_data_val[1] << 8) + gatt_svr_chr_ota_data_val[0];
            ESP_LOGI(LOG_TAG_GATT_SVR, "Packet size is: %d", packet_size);

            // clear the var
            num_pkgs_received = 0;

            // notify the client via BLE that the OTA has been acknowledged (or not)
            om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                       sizeof(gatt_svr_chr_ota_control_val));
            ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA request acknowledgement has been sent.");

            break;

            // this case is saying we are done with the comms and are closing the channel
        case SVR_CHR_OTA_CONTROL_DONE:

            // if needed, we can run checks of any kind here before telling the
            // other device if we are ok to terminate the channel

            // i.e. turn off all motors before closing connection
            gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_ACK;

            // or
//        gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_NAK;


            // notify the client via BLE that DONE has been acknowledged
            om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                       sizeof(gatt_svr_chr_ota_control_val));

            ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
            ESP_LOGD(LOG_TAG_GATT_SVR, "OTA DONE acknowledgement has been sent.");

            break;

        default:
            break;
    }


}

static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg) {
    int rc;
    uint8_t length = sizeof(gatt_svr_chr_ota_control_val);

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            // a client is reading the current value of ota control
            rc = os_mbuf_append(ctxt->om, &gatt_svr_chr_ota_control_val, length);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            break;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            // a client is writing a value to ota control
            rc = gatt_svr_chr_write(ctxt->om, 1, length,
                                    &gatt_svr_chr_ota_control_val, NULL);

            // write the new value control:
            update_ota_control(conn_handle);
            return rc;
            break;

        default:
            break;
    }

    // this shouldn't happen
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}


// this is where we will recieve the actual data over characteristic OTA Data
static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg) {
    int rc;
    esp_err_t err;

    // store the received data into gatt_svr_chr_ota_data_val
    rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_ota_data_val),
                            gatt_svr_chr_ota_data_val, NULL);

    ESP_LOGI(LOG_TAG_GATT_SVR, "Received packet data:%i, %i, %i, %i, %i", gatt_svr_chr_ota_data_val[0],
             gatt_svr_chr_ota_data_val[1], gatt_svr_chr_ota_data_val[2], gatt_svr_chr_ota_data_val[3],
             gatt_svr_chr_ota_data_val[4]);

    // Example command to set motor speeds and direction for 10 seconds
    MotorCommand command = {gatt_svr_chr_ota_data_val[0], gatt_svr_chr_ota_data_val[1],
                            gatt_svr_chr_ota_data_val[2], gatt_svr_chr_ota_data_val[3],
                            gatt_svr_chr_ota_data_val[4]};
    set_motor_command(command);

    num_pkgs_received++;
    ESP_LOGI(LOG_TAG_GATT_SVR, "Received packet %d", num_pkgs_received);

    return rc;
}

void gatt_svr_init() {
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);
}
