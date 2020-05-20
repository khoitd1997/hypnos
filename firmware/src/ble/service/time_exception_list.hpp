#ifndef _TIME_EXCEPTION_LIST_HPP
#define _TIME_EXCEPTION_LIST_HPP

#include <cstdint>

#include <array>

#include "ble_custom_characteristic_value_type.hpp"

// NOTE: If you are maintaing it in year 2038, need to change this to 64 bits
// time_t is 64 bits, that's way too many bits
typedef uint32_t unix_time_t;

struct TimeException {
  unix_time_t start_time;
  unix_time_t end_time;
  friend bool operator==(const TimeException &lhs, const TimeException &rhs);
};

class TimeExceptionList : public BleCustomCharacteristicValueType {
 private:
  static constexpr size_t MAX_ACTIVE_EXCEPTIONS = 4;

  std::array<TimeException, MAX_ACTIVE_EXCEPTIONS> _exception_list;
  size_t                                           _total_exception = 0;

 public:
  uint8_t size_in_bytes() const override;
  size_t  size() const;

  uint8_t max_size_in_bytes() const override {
    return static_cast<uint8_t>(sizeof(TimeException) * max_size());
  }
  constexpr size_t max_size() const { return _exception_list.max_size(); };

  bool is_full() const;
  bool is_empty() const;
  void set_empty();

  uint8_t *data() const override;

  bool replace(const uint8_t *buf, const size_t len) override;
  bool push(const TimeException &time_exception);
  bool get(const size_t index, TimeException &time_exception) const;

  friend bool operator==(const TimeExceptionList &lhs, const TimeExceptionList &rhs);
};

#endif  // ! _EXCEPTION_LIST_HPP