#ifndef _TWI_MODULE_HPP
#define _TWI_MODULE_HPP

#include <cstdint>

namespace twi {
  void init();

  void read(const uint8_t address, uint8_t* p_data, const uint8_t length);
  void write(const uint8_t  address,
             uint8_t const* p_data,
             const uint8_t  length,
             const bool     no_stop = false);
}  // namespace twi
#endif