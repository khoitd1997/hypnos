#ifndef _BAS_MODULE_HPP
#define _BAS_MODULE_HPP

#include <cstdint>

#include "nrf_drv_saadc.h"

namespace ble::bas {
  void adc_event_callback(nrf_drv_saadc_evt_t const* p_event, const uint16_t result_millivolt);

  void init();
}  // namespace ble::bas

#endif  // ! _BAS_MODULE_HPP