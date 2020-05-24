#include "time_hour_minute.hpp"

TimeHourMinute::TimeHourMinute(const uint8_t hour, const uint16_t minute) { set(hour, minute); }

void TimeHourMinute::set(const uint8_t hour, const uint16_t minute) {
  _data = (hour << 6) | minute;
}
void TimeHourMinute::get(uint8_t& hour, uint16_t& minute) const {
  hour   = getHour();
  minute = getMinute();
}
uint8_t  TimeHourMinute::getHour() const { return (_data >> 6) & 0b11111; }
uint16_t TimeHourMinute::getMinute() const { return _data & 0b111111; }

uint8_t* TimeHourMinute::data() const { return (uint8_t*)(&_data); }

uint8_t TimeHourMinute::max_size_in_bytes() const { return size_in_bytes(); }
uint8_t TimeHourMinute::size_in_bytes() const { return sizeof(time_hour_minute_t); }

bool TimeHourMinute::replace(const uint8_t* buf, const size_t len) {
  if (len != size_in_bytes()) { return false; }

  _data = *((const time_hour_minute_t*)(buf));

  return true;
}

bool operator==(const TimeHourMinute& lhs, const TimeHourMinute& rhs) {
  return lhs._data == rhs._data;
}

int TimeHourMinute::difftime(const TimeHourMinute& time_end, const TimeHourMinute& time_start) {
  return (time_end.getHour() - time_start.getHour()) * 3600 +
         (time_end.getMinute() - time_start.getMinute()) * 60;
}