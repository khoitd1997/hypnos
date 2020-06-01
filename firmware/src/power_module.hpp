#ifndef _POWER_MODULE_HPP
#define _POWER_MODULE_HPP

#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

namespace power {
  enum Wakeup_Reason { NONE = 0, USER_INPUT, WORK_PERIOD_END };

  void init(const nrfx_gpiote_pin_t user_input_pin, const nrfx_gpiote_pin_t rtc_interrupt_pin);

  Wakeup_Reason get_wakeup_reason();

  void run();
  void sleep(const bool enable_user_input_wakeup, const bool enable_rtc_wakeup);
}  // namespace power

#endif  // ! _POWER_MODULE_HPP