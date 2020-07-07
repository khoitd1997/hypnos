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

#include "nrf_pwr_mgmt.h"
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
#include "nrf_log_default_backends.h"

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
  in_config.pull                       = NRF_GPIO_PIN_NOPULL;

  const auto err_code = nrf_drv_gpiote_in_init(
      RESET_PIN, &in_config, [](nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
        nrf_drv_gpiote_in_event_disable(RESET_PIN);
        ble::pm::delete_all_bonds_unsafe();
        sd_nvic_SystemReset();
      });
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(RESET_PIN, true);
}

// APP_TIMER_DEF(m_timer_id);

int main(void) {
  {
    const auto err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
  }
  auto err_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(err_code);
  {
    const auto err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
  }

  configure_reset_pin();

  // need to initialize but not call nrf_drv_gpiote_in_event_enable otherwise
  // latch clear doesn't seem to work
  {
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    in_config.pull                       = NRF_GPIO_PIN_NOPULL;
    const auto err_code = nrf_drv_gpiote_in_init(RTC_INTERRUPT_INPUT_PIN, &in_config, nullptr);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_disable(RTC_INTERRUPT_INPUT_PIN);
  }
  {
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    in_config.pull                       = NRF_GPIO_PIN_NOPULL;
    const auto err_code = nrf_drv_gpiote_in_init(USER_INPUT_PIN, &in_config, nullptr);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_disable(USER_INPUT_PIN);
  }

  NRF_LOG_INFO("User input pin state: %d", nrf_gpio_pin_read(USER_INPUT_PIN));
  NRF_LOG_INFO("RTC input pin state: %d", nrf_gpio_pin_read(RTC_INTERRUPT_INPUT_PIN));
  NRF_LOG_INFO("Reset input pin state: %d", nrf_gpio_pin_read(RESET_PIN));

  if (nrf_gpio_pin_latch_get(USER_INPUT_PIN)) { NRF_LOG_INFO("user input latch set"); }
  if (nrf_gpio_pin_latch_get(RTC_INTERRUPT_INPUT_PIN)) { NRF_LOG_INFO("rtc interrupt latch set"); }
  if (nrf_gpio_pin_latch_get(RESET_PIN)) { NRF_LOG_INFO("reset latch set"); }
  NRF_LOG_INFO("Latch is %x", NRF_GPIO->LATCH);

  nrf_delay_ms(500);
  NRF_GPIO->LATCH = NRF_GPIO->LATCH;
  (void)NRF_TIMER1->EVENTS_COMPARE[0];

  // give the latch some time to clear
  NRF_LOG_INFO("Yo");
  NRF_LOG_INFO("Yo");
  NRF_LOG_INFO("Yo");
  NRF_LOG_INFO("Yo");
  NRF_LOG_FLUSH();

  while (nrf_gpio_pin_latch_get(USER_INPUT_PIN) ||
         nrf_gpio_pin_latch_get(RTC_INTERRUPT_INPUT_PIN) || nrf_gpio_pin_latch_get(RESET_PIN)) {
    // make sure the latch is cleared
  }

  NRF_LOG_INFO("Preparing to Sleep, latch: %d", NRF_GPIO->LATCH);
  NRF_LOG_FLUSH();

  nrf_drv_gpiote_in_event_enable(RTC_INTERRUPT_INPUT_PIN, true);
  nrf_drv_gpiote_in_event_enable(USER_INPUT_PIN, true);

  {
    auto err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Fetch the start address of the application RAM.
    uint32_t ram_start;
    err_code = nrf_sdh_ble_default_cfg_set(1, &ram_start);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    NRF_SDH_BLE_OBSERVER(m_ble_observer, 3, nullptr, NULL);

    ble_opt_t opt;

    memset(&opt, 0x00, sizeof(opt));
    opt.common_opt.conn_evt_ext.enable = 1;

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
    APP_ERROR_CHECK(err_code);
  }

  sd_power_system_off();
  while (1) {
    // in debug mode, sleep is emulated
  }
}

/**
 * @}
 */
