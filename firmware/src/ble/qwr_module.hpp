#ifndef _QWR_MODULE_HPP
#define _QWR_MODULE_HPP

#include <cstdint>

#include "nrf_ble_qwr.h"

namespace ble::qwr {
  nrf_ble_qwr_t *get();

  void init();

  void conn_handle_set(uint16_t conn_handle);
}  // namespace ble::qwr

#endif  // ! _QWR_MODULE_HPP