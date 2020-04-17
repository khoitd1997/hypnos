#ifndef _BMS_MODULE_HPP
#define _BMS_MODULE_HPP

#include <cstdint>

#include "nrf_ble_bms.h"

namespace ble::bms {
  uint16_t qwr_event_callback(nrf_ble_qwr_t *p_qwr, nrf_ble_qwr_evt_t *p_evt);

  void init();
  void conn_handle_set(const uint16_t conn_handle);
}  // namespace ble::bms

#endif  // ! _BMS_MODULE_HPP