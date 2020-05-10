#include "misc_module.hpp"

#include "bsp_btn_ble.h"

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
  namespace bsp {
    namespace {
      void bsp_event_handler(bsp_event_t event) {
        //   ret_code_t err_code;
        switch (event) {
            // case BSP_EVENT_DISCONNECT:
            //   err_code = sd_ble_gap_disconnect(ble::connection::get_handle(),
            //                                    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            //   if (err_code != NRF_ERROR_INVALID_STATE) { APP_ERROR_CHECK(err_code); }
            //   break;

          default:
            break;
        }
      }
    }  // namespace
    void init() {
      bsp_event_t startup_event;

      auto err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
      APP_ERROR_CHECK(err_code);

      err_code = bsp_btn_ble_init(NULL, &startup_event);
      APP_ERROR_CHECK(err_code);
    }
  }  // namespace bsp
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