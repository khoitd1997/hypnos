#include "misc_module.hpp"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "SEGGER_SYSVIEW.h"

namespace misc {
  namespace timer {
    void init() {
      const auto err_code = app_timer_init();
      APP_ERROR_CHECK(err_code);
    }

    void create(const app_timer_mode_t      mode,
                app_timer_id_t const*       id,
                app_timer_timeout_handler_t callback) {
      const auto err_code = app_timer_create(id, mode, callback);
      APP_ERROR_CHECK(err_code);
    }
  }  // namespace timer
  namespace log {
    void init() {
      const auto err_code = NRF_LOG_INIT(NULL);
      APP_ERROR_CHECK(err_code);

      NRF_LOG_DEFAULT_BACKENDS_INIT();
    }
  }  // namespace log

  namespace systemview {
    namespace {
      SEGGER_SYSVIEW_MODULE testModule{
          .sModule   = "M=testModule,0 testFunc some_num=%u",
          .NumEvents = 1,
      };
    }
    void init() {
      SEGGER_SYSVIEW_Conf();
      SEGGER_SYSVIEW_RegisterModule(&testModule);
    }
  }  // namespace systemview
}  // namespace misc