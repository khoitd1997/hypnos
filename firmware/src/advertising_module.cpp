#include "advertising_module.hpp"

#include "app_error.h"

#include "ble_advdata.h"
#include "ble_advertising.h"

#include "ble_srv_common.h"

#include "bsp_btn_ble.h"

#include "nrf_log.h"

#include "peer_manager.h"

#include "nrf_sdh_ble.h"

namespace {
  constexpr auto ADVERTISING_MODE = BLE_ADV_MODE_SLOW;

  //!< advertising interval (in units of 0.625 m
  constexpr uint32_t ADVERTISING_FAST_INTERVAL = 100;
  constexpr uint32_t ADVERTISING_SLOW_INTERVAL = 10000;

  constexpr uint32_t ADVERTISING_DURATION = 18000;  //!< units of 10 milliseconds.

  ble_uuid_t ADVERTISING_UUIDS[] = {
      {BLE_UUID_BMS_SERVICE, BLE_UUID_TYPE_BLE},
  };

  BLE_ADVERTISING_DEF(m_advertising);

  // TODO(khoi): Moved this to power module
  void sleep_mode_enter(void) {
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
  }

  // TODO(khoi): Move this somewhere else
  void delete_bonds_unsafe() {
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
  }

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
        NRF_LOG_INFO("Fast adverstising.");
        err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_ADV_EVT_SLOW:
        NRF_LOG_INFO("slow adverstising.");
        err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_2);
        APP_ERROR_CHECK(err_code);
        break;

      case BLE_ADV_EVT_IDLE:
        sleep_mode_enter();
        break;

      default:
        break;
    }
  }

  void advertising_error_handler(uint32_t nrf_error) { APP_ERROR_HANDLER(nrf_error); }
}  // namespace

namespace advertising {
  void init() {
    ble_advertising_init_t init{};

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.advdata.uuids_complete.uuid_cnt = sizeof(ADVERTISING_UUIDS) / sizeof(ADVERTISING_UUIDS[0]);
    init.advdata.uuids_complete.p_uuids  = ADVERTISING_UUIDS;

    // TODO(khoi): Enable when on final build
    //   init.config.ble_adv_on_disconnect_disabled = true;
    init.config.ble_adv_whitelist_enabled = false;

    // NOTE: still need to initialize fast o.t.w will not work
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = ADVERTISING_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout  = ADVERTISING_DURATION;

    init.config.ble_adv_slow_enabled  = true;
    init.config.ble_adv_slow_interval = ADVERTISING_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout  = ADVERTISING_DURATION;

    init.evt_handler   = advertising_event_handler;
    init.error_handler = advertising_error_handler;

    auto err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, CONN_CFG_TAG);
  }

  void start(const bool delete_bonds) {
    if (delete_bonds) {
      delete_bonds_unsafe();
      // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    } else {
      auto ret = ble_advertising_start(&m_advertising, ADVERTISING_MODE);
      APP_ERROR_CHECK(ret);
    }
  }
}  // namespace advertising