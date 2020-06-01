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

#include "rv3028.hpp"
#include "timetable_service_module.hpp"

#include "state_machine.hpp"

namespace ble {
  namespace {
    constexpr auto BLE_OBSERVER_PRIORITY = 3;
    volatile auto  m_connection_failed   = false;

    void gap_phy_update(const uint16_t conn_handle) {
      constexpr ble_gap_phys_t preferred_phys = {
          .tx_phys = BLE_GAP_PHY_2MBPS,
          .rx_phys = BLE_GAP_PHY_2MBPS,
      };
      const auto err_code = sd_ble_gap_phy_update(conn_handle, &preferred_phys);
      APP_ERROR_CHECK(err_code);
    }

    void ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context) {
      ret_code_t err_code = NRF_SUCCESS;
      //   NRF_LOG_INFO("ble event %u", p_ble_evt->header.evt_id);

      pm_handler_secure_on_connection(p_ble_evt);

      switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
          NRF_LOG_INFO("Connected");
          err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
          APP_ERROR_CHECK(err_code);
          connection::set_handle(p_ble_evt->evt.gap_evt.conn_handle);
          bms::conn_handle_set(connection::get_handle());
          qwr::conn_handle_set(connection::get_handle());

          gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle);
          m_connection_failed = false;
          break;

        case BLE_GAP_EVT_DISCONNECTED:
          NRF_LOG_INFO("Disconnected");
          connection::set_handle(BLE_CONN_HANDLE_INVALID);

          if ((not m_connection_failed) and (pm::is_bonded())) {
            NRF_LOG_INFO("Bonded and Success Disconnected");
            timetable_service::store_data_to_eeprom();
            if (state_machine::is_in_break()) {
              state_machine::end_work_period(true);
            } else {
              state_machine::start_work_period();
            }
          } else {
            NRF_LOG_INFO("Unbonded or failure Disconnected");
            advertising::start();
          }
          break;

        case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
          ble::timetable_service::current_unix_time_characteristic.set(RV3028::get().getUNIX());
          break;

        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
          m_connection_failed = true;
          NRF_LOG_DEBUG("GATT Timeout");
          err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                           BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_GAP_EVT_PHY_UPDATE: {
          ble_gap_evt_phy_update_t const *p_phy_evt = &p_ble_evt->evt.gap_evt.params.phy_update;

          if (p_phy_evt->status == BLE_HCI_STATUS_CODE_LMP_ERROR_TRANSACTION_COLLISION) {
            // Ignore LL collisions.
            NRF_LOG_DEBUG("LL transaction collision during PHY update.");
            break;
          }

          NRF_LOG_INFO("PHY update %s. PHY set to %d %d.",
                       (p_phy_evt->status == BLE_HCI_STATUS_CODE_SUCCESS) ? "accepted" : "rejected",
                       p_phy_evt->tx_phy,
                       p_phy_evt->rx_phy);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
          gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle);
        } break;

        default:
          // No implementation needed.
          break;
      }
    }
  }  // namespace

  void init() {
    auto err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Fetch the start address of the application RAM.
    uint32_t ram_start;
    err_code = nrf_sdh_ble_default_cfg_set(advertising::CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    NRF_SDH_BLE_OBSERVER(m_ble_observer, BLE_OBSERVER_PRIORITY, ble_event_handler, NULL);

    ble_opt_t opt;

    memset(&opt, 0x00, sizeof(opt));
    opt.common_opt.conn_evt_ext.enable = 1;

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble