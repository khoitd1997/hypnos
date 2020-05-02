#include "exception_list.hpp"

#include "doctest.h"

uint16_t ExceptionList::size_in_bytes() const {
  return static_cast<uint16_t>(SIZE_OF_ONE_EXCEPTION * _total_exception);
}
uint16_t ExceptionList::max_size_in_bytes() const {
  return static_cast<uint16_t>(SIZE_OF_ONE_EXCEPTION * MAX_ACTIVE_EXCEPTIONS);
}
size_t ExceptionList::size() const { return _total_exception; }

bool ExceptionList::is_full() const { return _total_exception == MAX_ACTIVE_EXCEPTIONS; }
bool ExceptionList::is_empty() const { return _total_exception == 0; }
void ExceptionList::set_empty() { _total_exception = 0; }

uint8_t *ExceptionList::data() const {
  return (uint8_t *)(const_cast<time_t *>(_raw_times.data()));
}

bool ExceptionList::push(const time_t &start_time, const time_t &end_time) {
  if (is_full()) { return false; }

  _raw_times.at(_total_exception * 2)     = start_time;
  _raw_times.at(_total_exception * 2 + 1) = end_time;
  ++_total_exception;
  return true;
}
bool ExceptionList::get(const size_t index, time_t &start_time, time_t &end_time) const {
  if (index >= _total_exception) { return false; }

  start_time = _raw_times.at(index * 2);
  end_time   = _raw_times.at(index * 2 + 1);
  return true;
}

int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }

TEST_CASE("testing the factorial function") {
  CHECK(factorial(1) == 1);
  CHECK(factorial(2) == 2);
  CHECK(factorial(3) == 6);
  CHECK(factorial(10) == 3628800);
}