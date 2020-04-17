#include "power_module.hpp"

#include "app_error.h"

#include "bsp_btn_ble.h"

#include "nrf_pwr_mgmt.h"

namespace power {
  void init() {
    const auto err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
  }

  void run() { nrf_pwr_mgmt_run(); }

  void sleep() {
    auto err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
  }
}  // namespace power