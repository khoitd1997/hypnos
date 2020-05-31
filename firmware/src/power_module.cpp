#include "power_module.hpp"

#include "app_error.h"

#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

namespace power {
  namespace {
    nrfx_gpiote_pin_t m_wakeup_pin;
  }

  void init(const nrfx_gpiote_pin_t wakeup_pin) {
    m_wakeup_pin = wakeup_pin;
    nrf_gpio_input_disconnect(m_wakeup_pin);

    const auto err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
  }

  void run() { nrf_pwr_mgmt_run(); }

  void sleep() {
#ifdef DEBUG_NRF
    auto pin_pull = NRF_GPIO_PIN_PULLUP;
#else
    auto pin_pull = NRF_GPIO_PIN_NOPULL;
#endif
    nrf_gpio_cfg_sense_input(m_wakeup_pin, pin_pull, NRF_GPIO_PIN_SENSE_LOW);

    // error code doesn't matter here, since system still sleep despite return
    // NOTE: Debug mode has fake sleep so will consume power normally
    sd_power_system_off();
#ifdef DEBUG_NRF
    NRF_LOG_INFO("Entering Debug Sleep");
    NRF_LOG_FLUSH();
    while (1) {
      // in debug mode, sleep is emulated
    }
#endif
  }
}  // namespace power