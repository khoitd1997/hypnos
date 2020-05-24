#ifndef _TIME_HOUR_MINUTE_T
#define _TIME_HOUR_MINUTE_T

#include <cstdint>

#include <array>

#include "ble_custom_characteristic_value_type.hpp"

struct TimeHourMinute : public BleCustomCharacteristicValueType {
 private:
  typedef uint16_t time_hour_minute_t;

  time_hour_minute_t _data = 0;

 public:
  TimeHourMinute() = default;
  TimeHourMinute(const uint8_t hour, const uint16_t minute);

  void set(const uint8_t hour, const uint16_t minute);
  void get(uint8_t& hour, uint16_t& minute) const;

  uint8_t* data() const override;
  uint8_t  max_size_in_bytes() const override;
  uint8_t  size_in_bytes() const override;
  bool     replace(const uint8_t* buf, const size_t len) override;

  friend bool operator==(const TimeHourMinute& lhs, const TimeHourMinute& rhs);
};

#endif