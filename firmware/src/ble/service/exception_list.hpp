#ifndef _EXCEPTION_LIST_HPP
#define _EXCEPTION_LIST_HPP

#include <cstdint>
#include <ctime>

#include <array>

class ExceptionList {
 private:
  static constexpr size_t MAX_ACTIVE_EXCEPTIONS = 4;
  static constexpr size_t SIZE_OF_ONE_EXCEPTION = sizeof(time_t) * 2;

  std::array<time_t, MAX_ACTIVE_EXCEPTIONS * 2> _raw_times;
  size_t                                        _total_exception = 0;

 public:
  uint16_t size_in_bytes() const;
  uint16_t max_size_in_bytes() const;
  size_t   size() const;

  bool is_full() const;
  bool is_empty() const;
  void set_empty();

  uint8_t *data() const;

  bool push(const time_t &start_time, const time_t &end_time);
  bool get(const size_t index, time_t &start_time, time_t &end_time) const;
};

#endif  // ! _EXCEPTION_LIST_HPP