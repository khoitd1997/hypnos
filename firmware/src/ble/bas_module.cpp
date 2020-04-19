#include "bas_module.hpp"

#include "app_error.h"

#include "ble_bas.h"

#include "nrf_sdh_ble.h"

#include "misc_module.hpp"

namespace ble::bas {
  namespace {
#define BATTERY_LEVEL_MEAS_INTERVAL                                                               \
  APP_TIMER_TICKS(120000) /**< Battery level measurement interval (ticks). This value corresponds \
                             to 120 seconds. */
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS 270
    BLE_BAS_DEF(m_bas);

    app_timer_id_t m_timer_id;

    void bas_event_handler(ble_bas_t* p_bas, ble_bas_evt_t* p_evt) {
      ret_code_t err_code;

      switch (p_evt->evt_type) {
        case BLE_BAS_EVT_NOTIFICATION_ENABLED:
          // Start battery timer
          err_code = app_timer_start(m_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, nullptr);
          APP_ERROR_CHECK(err_code);
          break;  // BLE_BAS_EVT_NOTIFICATION_ENABLED

        case BLE_BAS_EVT_NOTIFICATION_DISABLED:
          err_code = app_timer_stop(m_timer_id);
          APP_ERROR_CHECK(err_code);
          break;  // BLE_BAS_EVT_NOTIFICATION_DISABLED

        default:
          // No implementation needed.
          break;
      }
    }
  }  // namespace

  void adc_event_callback(nrf_drv_saadc_evt_t const* p_event, const uint16_t result_millivolt) {
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
      const auto batt_lvl_in_milli_volts = result_millivolt + DIODE_FWD_VOLT_DROP_MILLIVOLTS;
      const auto percentage_batt_lvl     = battery_level_in_percent(batt_lvl_in_milli_volts);

      const auto err_code =
          ble_bas_battery_level_update(&m_bas, percentage_batt_lvl, BLE_CONN_HANDLE_ALL);
      if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_INVALID_STATE) &&
          (err_code != NRF_ERROR_RESOURCES) && (err_code != NRF_ERROR_BUSY) &&
          (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {
        APP_ERROR_HANDLER(err_code);
      }
    }
  }

  void init() {
    ble_bas_init_t bas_init_obj{};

    bas_init_obj.evt_handler          = bas_event_handler;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = nullptr;
    bas_init_obj.initial_batt_level   = 100;

    bas_init_obj.bl_rd_sec        = SEC_OPEN;
    bas_init_obj.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init_obj.bl_report_rd_sec = SEC_OPEN;

    const auto err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);

    misc::timer::create(APP_TIMER_MODE_REPEATED, m_timer_id, [](void* context) {
      UNUSED_PARAMETER(context);

      const auto err_code = nrf_drv_saadc_sample();
      APP_ERROR_CHECK(err_code);
    });
  }
}  // namespace ble::bas