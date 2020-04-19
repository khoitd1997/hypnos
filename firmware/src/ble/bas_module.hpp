#ifndef _BAS_MODULE_HPP
#define _BAS_MODULE_HPP

#include <cstdint>

#include "nrf_drv_saadc.h"

namespace ble::bas {
  void init();

  void update();
}  // namespace ble::bas

#endif  // ! _BAS_MODULE_HPP