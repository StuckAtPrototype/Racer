/**
 * @file gap.c
 * @brief BLE GAP (Generic Access Profile) implementation
 * 
 * This file implements the BLE GAP functionality for the Racer3 device, including
 * device advertising, connection management, and event handling. It manages the
 * device's discoverability and connection state through the NimBLE stack.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "gap.h"
#include "led.h"

// Global variable to store the address type used for advertising
uint8_t addr_type;

// Forward declaration for the GAP event handler
int gap_event_handler(struct ble_gap_event *event, void *arg);

/**
 * @brief Start BLE advertising
 * 
 * This function configures and starts BLE advertising for the Racer3 device.
 * It sets up advertisement fields including device name, power level, and
 * discoverability flags, then starts the advertising process.
 */
void advertise() {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  int rc;

  memset(&fields, 0, sizeof(fields));

  // Set advertisement flags: general discoverability + BLE only (no BR/EDR)
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  // Include power level information in advertisement
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  // Include device name in advertisement
  fields.name = (uint8_t *)device_name;
  fields.name_len = strlen(device_name);
  fields.name_is_complete = 1;

  // Set the advertisement data fields
  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG_GAP, "Error setting advertisement data: rc=%d", rc);
    return;
  }

  // Configure and start advertising
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;  // Undirected advertising
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;   // General discoverability
  rc = ble_gap_adv_start(addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                         gap_event_handler, NULL);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG_GAP, "Error enabling advertisement data: rc=%d", rc);
    return;
  }
}

/**
 * @brief BLE reset callback function
 * 
 * This function is called when the BLE stack is reset. It logs the reset reason
 * for debugging purposes.
 * 
 * @param reason The reason code for the BLE reset
 */
void reset_cb(int reason) {
  ESP_LOGE(LOG_TAG_GAP, "BLE reset: reason = %d", reason);
}

/**
 * @brief BLE synchronization callback function
 * 
 * This function is called when the BLE stack is synchronized and ready to use.
 * It determines the best address type for advertising and starts the advertising
 * process.
 */
void sync_cb(void) {
  // Determine the best address type for advertising
  ble_hs_id_infer_auto(0, &addr_type);

  // Start advertising once the stack is ready
  advertise();
}

/**
 * @brief GAP event handler for BLE connection events
 * 
 * This function handles various BLE GAP events including connection establishment,
 * disconnection, advertising completion, subscription changes, and MTU updates.
 * It manages LED states and advertising behavior based on connection status.
 * 
 * @param event Pointer to the GAP event structure
 * @param arg User argument (unused)
 * @return 0 on success
 */
int gap_event_handler(struct ble_gap_event *event, void *arg) {
  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
      // A new connection was established or a connection attempt failed
      ESP_LOGI(LOG_TAG_GAP, "GAP: Connection %s: status=%d",
               event->connect.status == 0 ? "established" : "failed",
               event->connect.status);
      break;

    case BLE_GAP_EVENT_DISCONNECT:
      ESP_LOGD(LOG_TAG_GAP, "GAP: Disconnect: reason=%d\n",
               event->disconnect.reason);

      // Turn off the front LEDs when disconnected
      led_set_headlight_color(LED_COLOR_OFF);

      // Connection terminated; resume advertising
      advertise();
      break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
      ESP_LOGI(LOG_TAG_GAP, "GAP: adv complete");
      // Restart advertising when current advertising cycle completes
      advertise();
      break;

    case BLE_GAP_EVENT_SUBSCRIBE:
      ESP_LOGI(LOG_TAG_GAP, "GAP: Subscribe: conn_handle=%d",
               event->connect.conn_handle);
      break;

    case BLE_GAP_EVENT_MTU:
      ESP_LOGI(LOG_TAG_GAP, "GAP: MTU update: conn_handle=%d, mtu=%d",
               event->mtu.conn_handle, event->mtu.value);
      break;
  }

  return 0;
}

/**
 * @brief BLE host task for NimBLE stack
 * 
 * This function runs the NimBLE host stack in a FreeRTOS task. It handles
 * all BLE operations and returns only when nimble_port_stop() is executed.
 * 
 * @param param Task parameter (unused)
 */
void host_task(void *param) {
  // Run the NimBLE host stack (returns only when nimble_port_stop() is executed)
  nimble_port_run();
  // Clean up FreeRTOS resources when the stack stops
  nimble_port_freertos_deinit();
}
