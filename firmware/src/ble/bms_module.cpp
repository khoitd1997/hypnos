#include "bms_module.hpp"

#include "app_error.h"

#include "nrf_ble_bms.h"

#include "nrf_sdh_ble.h"

#include "nrf_log.h"

#include "pm_module.hpp"
#include "qwr_module.hpp"

#undef USE_AUTHORIZATION_CODE
// #define USE_AUTHORIZATION_CODE 1

namespace ble::bms {
  namespace {
#ifdef USE_AUTHORIZATION_CODE
    static uint8_t m_auth_code[]   = {'A', 'B', 'C', 'D'};  // 0x41, 0x42, 0x43, 0x44
    static int     m_auth_code_len = sizeof(m_auth_code);
#endif

    NRF_BLE_BMS_DEF(m_bms);

    void bms_error_handler(uint32_t nrf_error) { APP_ERROR_HANDLER(nrf_error); }

    void bms_event_handler(nrf_ble_bms_t *p_ess, nrf_ble_bms_evt_t *p_evt) {
      ret_code_t err_code;
      bool       is_authorized = true;

      switch (p_evt->evt_type) {
        case NRF_BLE_BMS_EVT_AUTH:
          NRF_LOG_DEBUG("Authorization request.");
#if USE_AUTHORIZATION_CODE
          if ((p_evt->auth_code.len != m_auth_code_len) ||
              (memcmp(m_auth_code, p_evt->auth_code.code, m_auth_code_len) != 0)) {
            is_authorized = false;
          }
#endif
          err_code = nrf_ble_bms_auth_response(&m_bms, is_authorized);
          APP_ERROR_CHECK(err_code);
      }
    }
  }  // namespace

  uint16_t qwr_event_callback(nrf_ble_qwr_t *p_qwr, nrf_ble_qwr_evt_t *p_evt) {
    return nrf_ble_bms_on_qwr_evt(&m_bms, p_qwr, p_evt);
  }

  void init() {
    nrf_ble_bms_init_t bms_init{};

    bms_init.evt_handler   = bms_event_handler;
    bms_init.error_handler = bms_error_handler;
#if USE_AUTHORIZATION_CODE
    bms_init.feature.delete_requesting_auth         = true;
    bms_init.feature.delete_all_auth                = true;
    bms_init.feature.delete_all_but_requesting_auth = true;
#else
    bms_init.feature.delete_requesting         = true;
    bms_init.feature.delete_all                = true;
    bms_init.feature.delete_all_but_requesting = true;
#endif
    bms_init.bms_feature_sec_req = SEC_JUST_WORKS;
    bms_init.bms_ctrlpt_sec_req  = SEC_JUST_WORKS;

    bms_init.p_qwr                            = qwr::get();
    bms_init.bond_callbacks.delete_requesting = [](const nrf_ble_bms_t *target_bms) {
      pm::delete_curr_conn_bond(target_bms->conn_handle);
    };
    bms_init.bond_callbacks.delete_all = [](const nrf_ble_bms_t *target_bms) {
      UNUSED_PARAMETER(target_bms);
      pm::delete_all_bonds();
    };
    bms_init.bond_callbacks.delete_all_except_requesting = [](const nrf_ble_bms_t *target_bms) {
      pm::delete_all_except_curr_bond(target_bms->conn_handle);
    };

    const auto err_code = nrf_ble_bms_init(&m_bms, &bms_init);
    APP_ERROR_CHECK(err_code);
  }

  void conn_handle_set(const uint16_t conn_handle) {
    const auto err_code = nrf_ble_bms_set_conn_handle(&m_bms, conn_handle);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::bms