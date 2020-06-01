#include "power_module.hpp"

#include "app_error.h"

#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "rv3028.hpp"

namespace power {
  namespace {
    nrfx_gpiote_pin_t m_user_input_pin;
    nrfx_gpiote_pin_t m_rtc_interrupt_pin;

    auto m_wakeup_reason = Wakeup_Reason::NONE;
  }  // namespace

  void init(const nrfx_gpiote_pin_t user_input_pin, const nrfx_gpiote_pin_t rtc_interrupt_pin) {
    m_user_input_pin    = user_input_pin;
    m_rtc_interrupt_pin = rtc_interrupt_pin;
    nrf_gpio_input_disconnect(m_user_input_pin);
    nrf_gpio_input_disconnect(rtc_interrupt_pin);

    if (nrf_gpio_pin_latch_get(m_user_input_pin)) { m_wakeup_reason = Wakeup_Reason::USER_INPUT; }
    if (nrf_gpio_pin_latch_get(m_rtc_interrupt_pin)) {
      m_wakeup_reason = Wakeup_Reason::WORK_PERIOD_END;
    }
    nrf_gpio_pin_latch_clear(m_user_input_pin);
    nrf_gpio_pin_latch_clear(m_rtc_interrupt_pin);

    const auto err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
  }

  Wakeup_Reason get_wakeup_reason() { return m_wakeup_reason; }

  void run() { nrf_pwr_mgmt_run(); }

  void sleep(const bool enable_user_input_wakeup, const bool enable_rtc_wakeup) {
    NRF_LOG_INFO("Sleeping");
    NRF_LOG_FLUSH();

    if (enable_user_input_wakeup) {
#ifdef DEBUG_NRF
      auto pin_pull = NRF_GPIO_PIN_PULLUP;
#else
      auto pin_pull = NRF_GPIO_PIN_NOPULL;
#endif
      nrf_gpio_cfg_sense_input(m_user_input_pin, pin_pull, NRF_GPIO_PIN_SENSE_LOW);
    }
    auto& rtc = RV3028::get();
    rtc.clearAlarmInterruptFlag();
    rtc.clearTimerInterruptFlag();
    if (enable_rtc_wakeup) {
      rtc.enableAlarmInterrupt();
      rtc.enableTimerInterrupt();
      nrf_gpio_cfg_sense_input(m_rtc_interrupt_pin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
    } else {
      rtc.disableAlarmInterrupt();
      rtc.disableTimerInterrupt();
    }

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