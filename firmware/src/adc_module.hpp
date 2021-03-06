#ifndef _ADC_MODULE_HPP
#define _ADC_MODULE_HPP

#include <cstdint>

namespace adc {
  void init();

  int16_t  sample_in_millivolt(const uint8_t channel);
  uint16_t sample_battery_in_millivolt();
}  // namespace adc

#endif  // _ADC_MODULE_HPP