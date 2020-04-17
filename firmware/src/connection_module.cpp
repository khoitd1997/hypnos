#include "connection_module.hpp"

#include "app_error.h"
#include "app_timer.h"

#include "ble_conn_params.h"
#include "ble_conn_state.h"

#include "nrf_ble_gatt.h"

namespace {
  constexpr uint32_t FIRST_CONN_PARAMS_UPDATE_DELAY =
      APP_TIMER_TICKS(15000);  //!< Time from initiating event (connect or start of notification) to
                               //!< first time sd_ble_gap_conn_param_update is called (5 seconds).
  constexpr uint32_t NEXT_CONN_PARAMS_UPDATE_DELAY =
      APP_TIMER_TICKS(5000);  //!< Time between each call to sd_ble_gap_conn_param_update after the
                              //!< first call (30 seconds).
  constexpr uint8_t MAX_CONN_PARAMS_UPDATE_COUNT = 3;  //!< Number of attempts before giving up the
                                                       //!< connection parameter negotiation.

  uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

  void connection_params_event_handler(ble_conn_params_evt_t *p_evt) {
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
      err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
      APP_ERROR_CHECK(err_code);
    }
  }

  void conn_params_error_handler(uint32_t nrf_error) { APP_ERROR_HANDLER(nrf_error); }
}  // namespace

namespace connection {
  void init() {
    ble_conn_params_init_t init{};

    init.p_conn_params                  = nullptr;
    init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    init.disconnect_on_fail             = false;
    init.evt_handler                    = connection_params_event_handler;
    init.error_handler                  = conn_params_error_handler;

    const auto err_code = ble_conn_params_init(&init);
    APP_ERROR_CHECK(err_code);
  }

  void     set_handle(const uint16_t handle) { m_conn_handle = handle; }
  uint16_t get_handle() { return m_conn_handle; }
}  // namespace connection