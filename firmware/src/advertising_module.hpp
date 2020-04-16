#ifndef _ADVERTISING_MODULE_HPP
#define _ADVERTISING_MODULE_HPP

#include <cstdint>

namespace advertising {
  constexpr uint8_t CONN_CFG_TAG = 1;

  void init();
  void start(const bool delete_bonds = false);
}  // namespace advertising

#endif  // ! _ADVERTISING_MODULE_HPP