#ifndef _POWER_MODULE_HPP
#define _POWER_MODULE_HPP

#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

namespace power {
  void init(const nrfx_gpiote_pin_t wakeup_pin);

  void run();
  void sleep();
}  // namespace power

#endif  // ! _POWER_MODULE_HPP