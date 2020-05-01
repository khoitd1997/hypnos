#include <stdint.h>

#ifndef _TIMETABLE_SERVICE_MODULE
#define _TIMETABLE_SERVICE_MODULE

namespace ble::timetable_service {
  void init();

  void value_update();
}  // namespace ble::timetable_service
#endif