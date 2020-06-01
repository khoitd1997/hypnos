#include "bas_module.hpp"

#include "app_error.h"

#include "ble_bas.h"

#include "nrf_sdh_ble.h"

#include "nrf_log.h"

#include "adc_module.hpp"
#include "misc_module.hpp"

namespace ble::bas {
  namespace {
    BLE_BAS_DEF(m_bas);

    uint8_t get_battery_percent() {
      return battery_level_in_percent(adc::sample_battery_in_millivolt());
    }
  }  // namespace

  void init() {
    NRF_LOG_INFO("Initializing BAS");

    ble_bas_init_t bas_init_obj{};

    bas_init_obj.evt_handler          = nullptr;
    bas_init_obj.support_notification = false;
    bas_init_obj.p_report_ref         = nullptr;
    bas_init_obj.initial_batt_level   = get_battery_percent();

    bas_init_obj.bl_rd_sec        = SEC_MITM;
    bas_init_obj.bl_cccd_wr_sec   = SEC_MITM;
    bas_init_obj.bl_report_rd_sec = SEC_MITM;

    const auto err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
  }

  void update() {
    const auto err_code =
        ble_bas_battery_level_update(&m_bas, get_battery_percent(), BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) && (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {
      APP_ERROR_HANDLER(err_code);
    }
  }
}  // namespace ble::bas