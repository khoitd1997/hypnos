/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>

#include "app_error.h"
#include "ble.h"

#include "ble_hci.h"
#include "fds.h"
#include "nordic_common.h"
#include "nrf.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "nrf_delay.h"

#include "ble_module.hpp"

#include "advertising_module.hpp"

#include "connection_module.hpp"

#include "gatt_module.hpp"

#include "gap_module.hpp"

#include "peer_manager_handler.h"
#include "pm_module.hpp"

#include "bms_module.hpp"
#include "qwr_module.hpp"

#include "bas_module.hpp"

#include "timetable_service_module.hpp"

#include "adc_module.hpp"

#include "power_module.hpp"

#include "gpio_module.hpp"

#include "computer_switch_module.hpp"

#include "rv3028.hpp"
#include "twi_module.hpp"

#include "misc_module.hpp"

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t* p_file_name) {
  NRF_LOG_FINAL_FLUSH();
  //!< Value used as error code on stack dump, can be used to identify stack location on
  //!< stack unwind.
  app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

// TODO(khoi): Remove this after development is done
#pragma GCC diagnostic ignored "-Wunused-function"

// static void reset() { pm::delete_all_bonds_unsafe(); }

// void testFunc() { SEGGER_SYSVIEW_RecordU32(0 + testModule.EventOffset, 5); }

APP_TIMER_DEF(m_timer_id);

static bool isInBreak() {
  auto&          rtc = RV3028::get();
  TimeHourMinute curr_hour_minute{rtc.getHours(), rtc.getMinutes()};
  const auto     curr_unix_time = rtc.getUNIX();

  const auto active_exceptions = ble::timetable_service::active_exceptions_characteristic.get();
  for (size_t i = 0; i < active_exceptions.size(); ++i) {
    TimeException e;
    active_exceptions.get(i, e);

    if ((curr_unix_time >= e.start_time) && (curr_unix_time <= e.end_time)) { return false; }
  }

  if ((TimeHourMinute::difftime(
           curr_hour_minute, ble::timetable_service::morning_curfew_characteristic.get()) <= 0) ||
      (TimeHourMinute::difftime(curr_hour_minute,
                                ble::timetable_service::night_curfew_characteristic.get()) >= 0)) {
    return true;
  }

  if (rtc.readAlarmInterruptFlag() || rtc.readTimerInterruptFlag()) { return true; }

  const auto break_start = rtc.getTimeStampInUNIX();
  if ((rtc.getUNIX() - break_start) <=
      ble::timetable_service::break_duration_characteristic.get() * 60) {
    return true;
  }

  return false;
}

constexpr nrfx_gpiote_pin_t COMPUTER_SWITCH_PIN = NRF_GPIO_PIN_MAP(0, 2);
constexpr nrfx_gpiote_pin_t WAKEUP_PIN          = NRF_GPIO_PIN_MAP(0, 9);
constexpr nrfx_gpiote_pin_t TIME_STAMP_PIN      = NRF_GPIO_PIN_MAP(0, 10);

int main(void) {
  misc::log::init();
  misc::systemview::init();
  misc::timer::init();
  power::init(WAKEUP_PIN);
  gpio::init();

  computer_switch::init(COMPUTER_SWITCH_PIN);

  twi::init();
  auto& rtc = RV3028::get();
  rtc.init(TIME_STAMP_PIN, true, true, false);
  //   rtc.enableExternalEventInterrupt(true, false);
  //   rtc.setToCompilerTime();
  //   rtc.enableClockOut(0);
  //   rtc.disableClockOut();

  rtc.disableAlarmInterrupt();
  rtc.clearAlarmInterruptFlag();
  rtc.disableTimerInterrupt();
  rtc.clearTimerInterruptFlag();
  rtc.clearClockOutputInterruptFlag();
  //   NRF_LOG_INFO("isInBreak: %d", isInBreak());
  rtc.setTimer(false, 1, 6, true, true, true);

  //   rtc.enableAlarmInterrupt(3, 19, 0, false, 4, true);
  //   rtc.enableInterruptControlledClockout(0);

  ble::init();
  adc::init();
  ble::gap::init();
  ble::gatt::init();

  ble::qwr::init();

  ble::timetable_service::init();

  ble::bms::init();
  ble::bas::init();

  ble::advertising::init();

  ble::connection::init();
  ble::pm::init();

  //   NRF_LOG_INFO("rtc: %s", rtc.stringTime());
  //   NRF_LOG_FLUSH();

  //   //   misc::timer::create(APP_TIMER_MODE_REPEATED, &m_timer_id, [](void* ctx) {
  //   testFunc();
  //   });
  //   //   app_timer_start(m_timer_id, APP_TIMER_TICKS(2000), nullptr);

  //   NRF_LOG_INFO("Bond Management example started.");

  ble::advertising::start();

  //   misc::timer::test_sleep();

  //   rtc.enableTimeStamp();
  //   rtc.clearTimeStamp();
  //   rtc.makeTimeStamp();
  //   NRF_LOG_INFO("%u %u", rtc.getUNIX(), rtc.getTimeStampInUNIX());

  //   NRF_LOG_INFO("sleeping");
  //   NRF_LOG_FLUSH();
  // TODO(khoi): Create timestamp when going to sleep when starting break
  //   power::sleep();

  for (;;) {
    // nrf_delay_ms(5000);
    // NRF_LOG_INFO("rtc: %s", rtc.stringTime());
    // NRF_LOG_FLUSH();

    if (!NRF_LOG_PROCESS()) { power::run(); }
  }
}

/**
 * @}
 */
