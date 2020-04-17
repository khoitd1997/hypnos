#ifndef _CONNECTION_MODULE_HPP
#define _CONNECTION_MODULE_HPP

#include <cstdint>

namespace ble::connection {
  void init();

  void     set_handle(const uint16_t handle);
  uint16_t get_handle();
}  // namespace ble::connection

#endif  // ! _CONNECTION_MODULE_HPP