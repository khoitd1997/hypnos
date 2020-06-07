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

#include "nrf_nvic.h"

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

#include "computer_switch_module.hpp"

#include "rv3028.hpp"
#include "twi_module.hpp"

#include "state_machine.hpp"

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

// void testFunc() { SEGGER_SYSVIEW_RecordU32(0 + testModule.EventOffset, 5); }

constexpr nrfx_gpiote_pin_t COMPUTER_SWITCH_PIN     = NRF_GPIO_PIN_MAP(0, 9);
constexpr nrfx_gpiote_pin_t USER_INPUT_PIN          = NRF_GPIO_PIN_MAP(0, 3);
constexpr nrfx_gpiote_pin_t RTC_INTERRUPT_INPUT_PIN = NRF_GPIO_PIN_MAP(0, 2);
constexpr nrfx_gpiote_pin_t RTC_TIME_STAMP_PIN      = NRF_GPIO_PIN_MAP(0, 10);
constexpr nrfx_gpiote_pin_t RESET_PIN               = NRF_GPIO_PIN_MAP(0, 11);

void configure_reset_pin() {
  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull                       = NRF_GPIO_PIN_PULLUP;

  const auto err_code = nrf_drv_gpiote_in_init(
      RESET_PIN, &in_config, [](nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
        nrf_drv_gpiote_in_event_disable(RESET_PIN);
        ble::pm::delete_all_bonds_unsafe();
        sd_nvic_SystemReset();
      });
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(RESET_PIN, true);
}

APP_TIMER_DEF(m_timer_id);

int main(void) {
  misc::log::init();
  misc::systemview::init();
  misc::timer::init();
  power::init(USER_INPUT_PIN, RTC_INTERRUPT_INPUT_PIN);
  {
    const auto err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
  }

  configure_reset_pin();

  computer_switch::init(COMPUTER_SWITCH_PIN);

  twi::init();
  auto& rtc = RV3028::get();
  rtc.init(RTC_TIME_STAMP_PIN, true, true, false);
  //   rtc.setToCompilerTime();
  // NOTE: MCU automatically switches to external LFCLK when it's available
  rtc.enableClockOut(0);
  rtc.enableTimeStamp();
  rtc.disableAlarmInterrupt();
  rtc.disableTimerInterrupt();

  ble::init();
  adc::init();
  ble::gap::init();
  ble::gatt::init();
  ble::advertising::init();
  ble::connection::init();
  ble::pm::init();
  //   ble::pm::delete_all_bonds_unsafe();

  ble::qwr::init();

  ble::timetable_service::init();

  ble::bms::init();
  ble::bas::init();

  NRF_LOG_PROCESS();
  power::sleep(false, true);

  const auto wakeup_reason = power::get_wakeup_reason();
  NRF_LOG_INFO("Wakeup Reason: %d", wakeup_reason);

  if ((not ble::pm::is_bonded()) or (power::Wakeup_Reason::WORK_PERIOD_END == wakeup_reason) or
      (power::Wakeup_Reason::NONE == wakeup_reason)) {
    ble::advertising::start();
  } else if (power::Wakeup_Reason::USER_INPUT == wakeup_reason) {
    if (state_machine::is_in_break()) {
      state_machine::end_work_period(false);
    } else {
      state_machine::start_work_period();
    }
  }

  for (;;) {
    if (!NRF_LOG_PROCESS()) { power::run(); }
  }
}

/**
 * @}
 */
