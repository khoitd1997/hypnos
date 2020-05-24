#include "gpio_module.hpp"

#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

#include "app_error.h"

#include "rv3028.hpp"

namespace gpio {
  namespace {
    // constexpr nrfx_gpiote_pin_t PIN_OUT = NRF_GPIO_PIN_MAP(0, 13);

    // void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    //   //   nrf_drv_gpiote_out_clear(PIN_OUT);
    //   //   nrf_drv_gpiote_out_set(PIN_OUT);
    //   nrf_drv_gpiote_out_set(PIN_OUT);

    //   auto& rtc = RV3028::get();
    //   rtc.disableAlarmInterrupt();
    //   rtc.clearAlarmInterruptFlag();
    //   rtc.disableTimerInterrupt();
    //   rtc.clearTimerInterruptFlag();
    // }
  }  // namespace
  void init() {
    ret_code_t err_code;

    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

    // err_code = nrf_drv_gpiote_out_init(PIN_OUT, &out_config);
    // APP_ERROR_CHECK(err_code);

    // nrf_drv_gpiote_out_clear(PIN_OUT);

    // nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    // in_config.pull                       = NRF_GPIO_PIN_PULLUP;

    // err_code = nrf_drv_gpiote_in_init(PIN_IN, &in_config, in_pin_handler);
    // APP_ERROR_CHECK(err_code);

    // nrf_drv_gpiote_in_event_enable(PIN_IN, true);
  }
}  // namespace gpio