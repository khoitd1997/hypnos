#include "misc_module.hpp"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_sdh.h"
#include "nrf_soc.h"

#include "SEGGER_SYSVIEW.h"

namespace misc {
  namespace timer {
    namespace {
      APP_TIMER_DEF(m_sleep_timer_id);
      volatile auto m_sleep_timer_is_finished = true;

      auto m_sleep_test_running = false;
    }  // namespace

    void init() {
      const auto err_code = app_timer_init();
      APP_ERROR_CHECK(err_code);

      create(APP_TIMER_MODE_SINGLE_SHOT, &m_sleep_timer_id, [](void* ctx) {
        UNUSED_PARAMETER(ctx);
        m_sleep_timer_is_finished = true;
      });
    }

    void create(const app_timer_mode_t      mode,
                app_timer_id_t const*       id,
                app_timer_timeout_handler_t callback) {
      const auto err_code = app_timer_create(id, mode, callback);
      APP_ERROR_CHECK(err_code);
    }

    void sleep(const uint32_t milliseconds) {
      ASSERT(m_sleep_timer_is_finished);
      ASSERT(nrf_sdh_is_enabled());  // the wait functio needs softdevice

      m_sleep_timer_is_finished = false;
      auto ret = app_timer_start(m_sleep_timer_id, APP_TIMER_TICKS(milliseconds), nullptr);
      APP_ERROR_CHECK(ret);

      while (!m_sleep_timer_is_finished) {
        if (m_sleep_test_running) { NRF_LOG_INFO("going to sleep"); }

        ret = sd_app_evt_wait();
        APP_ERROR_CHECK(ret);
      }
    }
    void test_sleep() {
      // timer will interrupt the sleep but it should go back to sleep after
      m_sleep_test_running = true;
      APP_TIMER_DEF(interrupting_timer_id);
      create(APP_TIMER_MODE_SINGLE_SHOT, &interrupting_timer_id, [](void* ctx) {
        UNUSED_PARAMETER(ctx);
        NRF_LOG_INFO("interrupting timer");
      });
      const auto ret = app_timer_start(interrupting_timer_id, APP_TIMER_TICKS(5000), nullptr);
      APP_ERROR_CHECK(ret);

      NRF_LOG_INFO("starting sleep");
      NRF_LOG_FLUSH();
      sleep(10000);
      NRF_LOG_INFO("ending sleep");
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