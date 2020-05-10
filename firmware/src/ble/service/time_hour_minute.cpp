#include "time_hour_minute.hpp"

#include <cassert>

TimeHourMinute::TimeHourMinute(const uint8_t hour, const uint16_t minute) { set(hour, minute); }

void TimeHourMinute::set(const uint8_t hour, const uint16_t minute) {
  _data = (hour << 6) | minute;
}
void TimeHourMinute::get(uint8_t& hour, uint16_t& minute) {
  hour   = (_data >> 6) & 0b11111;
  minute = _data & 0b111111;
}

uint8_t* TimeHourMinute::data() const { return (uint8_t*)(&_data); }

uint16_t TimeHourMinute::max_size_in_bytes() const { return size_in_bytes(); }
uint16_t TimeHourMinute::size_in_bytes() const { return sizeof(time_hour_minute_t); }

bool TimeHourMinute::replace(const uint8_t* buf, const size_t len) {
  if (len != size_in_bytes()) { return false; }

  _data = *((const time_hour_minute_t*)(buf));

  return true;
}