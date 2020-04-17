#include "ble_module.hpp"

#include "app_error.h"

#include "ble.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "bsp_btn_ble.h"

#include "nrf_log.h"

#include "advertising_module.hpp"

#include "connection_module.hpp"

#include "peer_manager_handler.h"
#include "pm_module.hpp"

#include "bms_module.hpp"
#include "qwr_module.hpp"

namespace ble {
  namespace {
    constexpr auto BLE_OBSERVER_PRIORITY =
        3;  //!< Application's BLE observer priority. You shouldn't need to modify this value

    void ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context) {
      ret_code_t err_code = NRF_SUCCESS;

      pm_handler_secure_on_connection(p_ble_evt);

      switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
          NRF_LOG_INFO("Connected");
          err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
          APP_ERROR_CHECK(err_code);
          connection::set_handle(p_ble_evt->evt.gap_evt.conn_handle);
          bms::conn_handle_set(connection::get_handle());
          qwr::conn_handle_set(connection::get_handle());
          break;

        case BLE_GAP_EVT_DISCONNECTED:
          NRF_LOG_INFO("Disconnected");
          pm::delete_disconnected_bonds();
          connection::set_handle(BLE_CONN_HANDLE_INVALID);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
          NRF_LOG_DEBUG("PHY update request.");
          ble_gap_phys_t const phys = {
              .tx_phys = BLE_GAP_PHY_AUTO,
              .rx_phys = BLE_GAP_PHY_AUTO,
          };
          err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
          APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
          // Disconnect on GATT Client timeout event.
          NRF_LOG_DEBUG("GATT Client Timeout.");
          err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                           BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_GATTS_EVT_TIMEOUT:
          // Disconnect on GATT Server timeout event.
          NRF_LOG_DEBUG("GATT Server Timeout.");
          err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                           BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
          APP_ERROR_CHECK(err_code);
          break;

        default:
          // No implementation needed.
          break;
      }
    }
  }  // namespace

  void init() {
    auto err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code           = nrf_sdh_ble_default_cfg_set(advertising::CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, BLE_OBSERVER_PRIORITY, ble_event_handler, NULL);
  }
}  // namespace ble