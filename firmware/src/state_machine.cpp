#include "state_machine.hpp"

#include <algorithm>

#include "rv3028.hpp"

#include "timetable_service_module.hpp"

#include "computer_switch_module.hpp"
#include "power_module.hpp"

#include "app_error.h"
#include "nrf_log.h"

namespace state_machine {
  bool is_in_break() {
    auto&          rtc = RV3028::get();
    TimeHourMinute curr_hour_minute{rtc.getHours(), rtc.getMinutes()};
    const auto     curr_unix_time = rtc.getUNIX();

    const auto active_exceptions = ble::timetable_service::active_exceptions_characteristic.get();
    if (active_exceptions.is_in_time_exception(curr_unix_time)) { return false; }

    if ((TimeHourMinute::difftime(
             curr_hour_minute, ble::timetable_service::morning_curfew_characteristic.get()) <= 0) ||
        (TimeHourMinute::difftime(
             curr_hour_minute, ble::timetable_service::night_curfew_characteristic.get()) >= 0)) {
      return true;
    }

    if (power::Wakeup_Reason::WORK_PERIOD_END == power::get_wakeup_reason()) { return true; }

    const auto break_start = rtc.getTimeStampInUNIX();
    if ((rtc.getUNIX() - break_start) <=
        ble::timetable_service::break_duration_characteristic.get() * 60) {
      return true;
    }

    return false;
  }

  void start_work_period() {
    NRF_LOG_INFO("Starting Work Period");
    auto&      rtc               = RV3028::get();
    const auto curr_unix_time    = rtc.getUNIX();
    uint16_t   timer_value       = 0;
    const auto active_exceptions = ble::timetable_service::active_exceptions_characteristic.get();
    TimeException e;
    if (active_exceptions.get_active_exception(curr_unix_time, e)) {
      timer_value = e.end_time - curr_unix_time;
    } else {
      timer_value = ble::timetable_service::work_duration_characteristic.get();
    }
    rtc.setTimer(false, 1, timer_value, true, true, true);

    const auto night_curfew = ble::timetable_service::night_curfew_characteristic.get();
    rtc.enableAlarmInterrupt(night_curfew.getMinute(), night_curfew.getHour(), 0, false, 4, false);

    power::sleep(false, true);
  }
  void end_work_period(const bool create_time_stamp) {
    NRF_LOG_INFO("Ending Work Period");
    if (create_time_stamp) { RV3028::get().createTimeStamp(); }

    computer_switch::off();
    power::sleep(true, false);
  }
}  // namespace state_machine