#include <cstdint>

#include "ble_characteristic.hpp"
#include "time_exception_list.hpp"

#ifndef _TIMETABLE_SERVICE_MODULE
#define _TIMETABLE_SERVICE_MODULE

namespace ble::timetable_service {
  typedef uint16_t time_hour_minute_t;

  extern BleCharacteristic<time_hour_minute_t>& morning_curfew_characteristic;
  extern BleCharacteristic<time_hour_minute_t>& night_curfew_characteristic;

  extern BleCharacteristic<uint8_t>& work_duration_characteristic;
  extern BleCharacteristic<uint8_t>& break_duration_characteristic;

  extern BleCharacteristic<TimeExceptionList>& active_exceptions_characteristic;

  extern BleCharacteristic<uint8_t>& tokens_left_characteristic;

  void init();

}  // namespace ble::timetable_service
#endif