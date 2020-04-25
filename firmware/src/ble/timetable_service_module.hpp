#include <stdint.h>

#ifndef _TIMETABLE_SERVICE_MODULE
#define _TIMETABLE_SERVICE_MODULE

namespace ble::timetable_service {
  void init();

  void ble_cus_custom_value_update(uint8_t custom_value);
}  // namespace ble::timetable_service
#endif