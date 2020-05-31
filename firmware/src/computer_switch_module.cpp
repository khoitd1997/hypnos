#include "computer_switch_module.hpp"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "nrf_delay.h"

namespace computer_switch {
  namespace {
    nrfx_gpiote_pin_t m_switch_pin;
  }

  void init(const nrfx_gpiote_pin_t switch_pin) {
    m_switch_pin = switch_pin;

    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    const auto                  err_code   = nrf_drv_gpiote_out_init(m_switch_pin, &out_config);
    APP_ERROR_CHECK(err_code);
  }

  void off() {
    nrf_drv_gpiote_out_set(m_switch_pin);
    nrf_delay_us(200);
    nrf_drv_gpiote_out_clear(m_switch_pin);
  }
}  // namespace computer_switch