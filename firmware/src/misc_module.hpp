#ifndef _MISC_MODULE_HPP
#define _MISC_MODULE_HPP

#include "app_timer.h"

namespace misc {
  namespace timer {
    void init();

    void create(const app_timer_mode_t      mode,
                app_timer_id_t&             id,
                app_timer_timeout_handler_t callback);
  }  // namespace timer
  namespace bsp {
    void init();
  }
  namespace log {
    void init();
  }
}  // namespace misc

#endif  // ! _MISC_MODULE_HPP