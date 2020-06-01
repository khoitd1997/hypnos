#include "time_exception_list.hpp"

#include "doctest.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <iostream>
#endif

bool operator==(const TimeException &lhs, const TimeException &rhs) {
  return (lhs.start_time == rhs.start_time) && (lhs.end_time == rhs.end_time);
}

namespace {
  void test_get_equal(TimeExceptionList &  exception_list,
                      const size_t         index,
                      const TimeException &target_exception) {
#ifndef DOCTEST_CONFIG_DISABLE
    TimeException e;
    REQUIRE(exception_list.get(index, e));
    CHECK(e.start_time == target_exception.start_time);
    CHECK(e.end_time == target_exception.end_time);
#else
    (void)(exception_list);
    (void)(index);
    (void)(target_exception);
#endif
  }
}  // namespace

uint8_t TimeExceptionList::size_in_bytes() const {
  return static_cast<uint8_t>(sizeof(TimeException) * _total_exception);
}
size_t TimeExceptionList::size() const { return _total_exception; }

bool TimeExceptionList::is_full() const { return _total_exception == max_size(); }
bool TimeExceptionList::is_empty() const { return _total_exception == 0; }
void TimeExceptionList::set_empty() { _total_exception = 0; }

uint8_t *TimeExceptionList::data() const {
  return (uint8_t *)(const_cast<TimeException *>(_exception_list.data()));
}

TEST_CASE("replace() method") {
  TimeExceptionList exception_list;
  auto              counter = 0;
  for (auto i = 0; i < exception_list.max_size(); ++i) {
    REQUIRE(exception_list.push({counter, counter + 1}));
    counter += 2;
  }

  SUBCASE("replace with normal array works") {
    constexpr size_t test_normal_array_size                    = exception_list.max_size() * 2;
    unix_time_t      test_normal_array[test_normal_array_size] = {0};
    for (auto i = 0; i < test_normal_array_size; i += 2) {
      test_normal_array[i]     = counter;
      test_normal_array[i + 1] = counter + 1;
      counter += 2;
    }
    REQUIRE(exception_list.replace((uint8_t *)(test_normal_array), sizeof(test_normal_array)));
    for (auto i = 0, j = 0; i < exception_list.max_size(); ++i, j += 2) {
      test_get_equal(exception_list, i, {test_normal_array[j], test_normal_array[j + 1]});
    }
  }
  SUBCASE("replace with too many elements errors") {
    unix_time_t test_too_big_array[exception_list.max_size() * 2 + 2] = {0};
    REQUIRE(
        not exception_list.replace((uint8_t *)(test_too_big_array), sizeof(test_too_big_array)));
    test_get_equal(exception_list, 0, {0, 1});
  }
  SUBCASE("replace with corrupt data errors") {
    unix_time_t test_corrupt_array[exception_list.max_size() * 2 - 1] = {0};
    REQUIRE(
        not exception_list.replace((uint8_t *)(test_corrupt_array), sizeof(test_corrupt_array)));
    test_get_equal(exception_list, 0, {0, 1});
  }
  SUBCASE("replace with none works") {
    unix_time_t test_empty_array[] = {};
    REQUIRE(exception_list.replace((uint8_t *)(test_empty_array), sizeof(test_empty_array)));
    CHECK(exception_list.is_empty());
    TimeException time_exception;
    CHECK(not exception_list.get(0, time_exception));
  }
  SUBCASE("replace with less elements works") {}
  SUBCASE("replace with more elements works") {}
}
bool TimeExceptionList::replace(const uint8_t *buf, const size_t len) {
  if (len > max_size_in_bytes() or (len % sizeof(TimeException) != 0)) { return false; }

  set_empty();
  if (len > 0) {
    const auto total_exception = len / sizeof(TimeException);
    auto       time_pt         = (const TimeException *)(buf);

    size_t counter = 0;
    while (counter != total_exception) {
      push({time_pt->start_time, time_pt->end_time});

      ++time_pt;
      ++counter;
    }
  }

  return true;
}

TEST_CASE("push() method") {
  TimeExceptionList exception_list;

  for (auto i = 0; i < exception_list.max_size(); ++i) {
    REQUIRE(exception_list.push({i, i + 1}));
    test_get_equal(exception_list, i, {i, i + 1});
  }

  REQUIRE(not exception_list.push({100, 101}));
}
bool TimeExceptionList::push(const TimeException &time_exception) {
  if (is_full()) { return false; }

  _exception_list.at(_total_exception) = time_exception;
  ++_total_exception;
  return true;
}

TEST_CASE("get() method") {
  TimeExceptionList exception_list;
  REQUIRE(exception_list.is_empty());

  TimeException time_exception;
  CHECK(not exception_list.get(0, time_exception));

  unix_time_t start_time = 5;
  unix_time_t end_time   = 10;
  REQUIRE(exception_list.push({start_time, end_time}));
  test_get_equal(exception_list, 0, {start_time, end_time});

  CHECK(not exception_list.get(1, time_exception));
}
bool TimeExceptionList::get(const size_t index, TimeException &time_exception) const {
  if (index >= _total_exception) { return false; }

  time_exception = _exception_list.at(index);
  return true;
}

bool TimeExceptionList::is_in_time_exception(const unix_time_t curr_time) const {
  for (size_t i = 0; i < size(); ++i) {
    TimeException e;
    get(i, e);

    if ((curr_time >= e.start_time) && (curr_time <= e.end_time)) { return true; }
  }

  return false;
}

bool TimeExceptionList::get_active_exception(const unix_time_t curr_time,
                                             TimeException &   exception) const {
  for (size_t i = 0; i < size(); ++i) {
    TimeException e;
    get(i, e);

    if ((curr_time >= e.start_time) && (curr_time <= e.end_time)) {
      exception = e;
      return true;
    }
  }

  return false;
}

bool operator==(const TimeExceptionList &lhs, const TimeExceptionList &rhs) {
  if (lhs.size() != rhs.size()) { return false; }
  for (size_t i = 0; i < lhs.size(); ++i) {
    TimeException lhs_e;
    lhs.get(i, lhs_e);
    TimeException rhs_e;
    rhs.get(i, rhs_e);

    if (!(lhs_e == rhs_e)) { return false; }
  }

  return true;
}