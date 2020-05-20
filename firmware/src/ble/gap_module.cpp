#include "gap_module.hpp"

#include <cstring>

#include "app_error.h"

#include "nrf_ble_gatt.h"
#include "nrf_sdh_ble.h"

namespace ble::gap {
  namespace {
    constexpr char DEVICE_NAME[] = "Hypnos";

    constexpr uint16_t MIN_CONN_INTERVAL = MSEC_TO_UNITS(15, UNIT_1_25_MS);
    constexpr uint16_t MAX_CONN_INTERVAL = MSEC_TO_UNITS(320, UNIT_1_25_MS);
    constexpr uint16_t SLAVE_LATENCY     = 0;
    constexpr uint16_t CONN_SUP_TIMEOUT  = MSEC_TO_UNITS(4000, UNIT_10_MS);
  }  // namespace

  void init() {
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params{};
    ble_gap_conn_sec_mode_t sec_mode{};

    sec_mode.sm = 1;
    sec_mode.lv = 3;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code =
        sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
    APP_ERROR_CHECK(err_code);

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    static ble_opt_t m_static_pin_option;

    static uint8_t passkey[] = "123455";

    m_static_pin_option.gap_opt.passkey.p_passkey = &passkey[0];

    err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &m_static_pin_option);

    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::gap