#ifndef _COMPUTER_SWITCH_MODULE
#define _COMPUTER_SWITCH_MODULE

#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

namespace computer_switch {
  void init(const nrfx_gpiote_pin_t switch_pin);

  void off();
}  // namespace computer_switch

#endif