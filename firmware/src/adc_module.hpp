#ifndef _ADC_MODULE_HPP
#define _ADC_MODULE_HPP

#include <cstdint>

namespace adc {
  void init();

  uint16_t sample_in_millivolt();
}  // namespace adc

#endif  // _ADC_MODULE_HPP