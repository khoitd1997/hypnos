#include "advertising_module.hpp"

#include "app_error.h"

#include "ble_advdata.h"
#include "ble_advertising.h"

#include "ble_srv_common.h"

#include "bsp_btn_ble.h"

#include "nrf_log.h"

#include "nrf_sdh_ble.h"

#include "computer_switch_module.hpp"
#include "pm_module.hpp"
#include "power_module.hpp"

#include "state_machine.hpp"

#include "timetable_service_module.hpp"

namespace ble::advertising {
  namespace {
    // constexpr auto ADVERTISING_MODE = BLE_ADV_MODE_SLOW;
    constexpr auto ADVERTISING_MODE = BLE_ADV_MODE_FAST;

    constexpr uint32_t ADVERTISING_FAST_INTERVAL = MSEC_TO_UNITS(62.5, UNIT_0_625_MS);
    constexpr uint32_t ADVERTISING_SLOW_INTERVAL = MSEC_TO_UNITS(6250, UNIT_0_625_MS);

    constexpr uint32_t ADVERTISING_DURATION = MSEC_TO_UNITS(180000, UNIT_10_MS);

    ble_uuid_t ADVERTISING_UUIDS[] = {
        {BLE_UUID_BMS_SERVICE, BLE_UUID_TYPE_BLE}, {BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE},
        // {CUSTOM_SERVICE_UUID, BLE_UUID_TYPE_VENDOR_BEGIN},
    };

    BLE_ADVERTISING_DEF(m_advertising);

    void advertising_event_handler(ble_adv_evt_t ble_adv_evt) {
      ret_code_t err_code;

      switch (ble_adv_evt) {
        case BLE_ADV_EVT_DIRECTED_HIGH_DUTY:
          NRF_LOG_INFO("directed high duty");
          err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_ADV_EVT_DIRECTED:
          NRF_LOG_INFO("directed");
          err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_ADV_EVT_FAST:
          //   NRF_LOG_INFO("Fast adverstising.");
          err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_ADV_EVT_SLOW:
          //   NRF_LOG_INFO("slow adverstising.");
          err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_2);
          APP_ERROR_CHECK(err_code);
          break;

        case BLE_ADV_EVT_IDLE:
          NRF_LOG_INFO("Idle advertising");
          if (not pm::is_bonded()) {
            start();
          } else {
            state_machine::end_work_period(true);
          }
          break;

        default:
          break;
      }
    }

    void advertising_error_handler(uint32_t nrf_error) { APP_ERROR_HANDLER(nrf_error); }
  }  // namespace

  void init() {
    ble_advertising_init_t init{};

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.advdata.uuids_complete.uuid_cnt = sizeof(ADVERTISING_UUIDS) / sizeof(ADVERTISING_UUIDS[0]);
    init.advdata.uuids_complete.p_uuids  = ADVERTISING_UUIDS;

    init.config.ble_adv_on_disconnect_disabled = true;
    init.config.ble_adv_whitelist_enabled      = false;

    // NOTE: still need to initialize fast o.t.w will not work
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = ADVERTISING_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout  = ADVERTISING_DURATION;

    init.config.ble_adv_slow_enabled  = true;
    init.config.ble_adv_slow_interval = ADVERTISING_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout  = ADVERTISING_DURATION;

    init.config.ble_adv_extended_enabled = true;
    init.config.ble_adv_secondary_phy    = BLE_GAP_PHY_2MBPS;
    init.config.ble_adv_primary_phy      = BLE_GAP_PHY_1MBPS;

    init.evt_handler   = advertising_event_handler;
    init.error_handler = advertising_error_handler;

    auto err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, CONN_CFG_TAG);
  }

  void start() {
    NRF_LOG_INFO("Started Advertising %d", pm::is_bonded());
    auto ret = ble_advertising_start(&m_advertising, ADVERTISING_MODE);
    APP_ERROR_CHECK(ret);
  }
}  // namespace ble::advertising