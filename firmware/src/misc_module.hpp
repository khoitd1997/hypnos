#ifndef _MISC_MODULE_HPP
#define _MISC_MODULE_HPP

#include "app_timer.h"

namespace misc {
  namespace timer {
    void init();

    void create(const app_timer_mode_t      mode,
                app_timer_id_t const*       id,
                app_timer_timeout_handler_t callback);

    void sleep(const uint32_t milliseconds);
    void test_sleep();
  }  // namespace timer
  namespace log {
    void init();
  }
  namespace systemview {
    void init();
  }  // namespace systemview
}  // namespace misc

#endif  // ! _MISC_MODULE_HPP