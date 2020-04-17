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
#include <string.h>
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"

#include "ble_hci.h"
#include "bsp_btn_ble.h"
#include "fds.h"
#include "nordic_common.h"
#include "nrf.h"

#include "nrf_ble_gatt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_module.hpp"

#include "advertising_module.hpp"

#include "connection_module.hpp"

#include "peer_manager_handler.h"
#include "pm_module.hpp"

#include "bms_module.hpp"
#include "qwr_module.hpp"

#include "power_module.hpp"

#define DEVICE_NAME "Hypnos"  //!< Name of device. Will be included in the advertising data.
#define MANUFACTURER_NAME \
  "KhoiTrinh"  //!< Manufacturer. Will be passed to Device Information Service.

#define SECOND_10_MS_UNITS 100  //!< Definition of 1 second, when 1 unit is 10 ms.
#define MIN_CONN_INTERVAL \
  7  //!< Minimum acceptable connection interval (0.25 seconds), Connection interval uses 1.25 ms
     //!< units.
#define MAX_CONN_INTERVAL \
  400  //!< Maximum acceptable connection interval (0.5 second), Connection interval uses 1.25 ms
       //!< units.
#define SLAVE_LATENCY 0  //!< Slave latency.
#define CONN_SUP_TIMEOUT \
  (4 * SECOND_10_MS_UNITS)  //!< Connection supervisory timeout (4 seconds), Supervision Timeout
                            //!< uses 10 ms units.

#define DEAD_BEEF \
  0xDEADBEEF  //!< Value used as error code on stack dump, can be used to identify stack location on
              //!< stack unwind.

NRF_BLE_GATT_DEF(m_gatt);  //!< GATT module instance.

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
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void timers_init(void) {
  ret_code_t err_code;

  // Initialize timer module, making it use the scheduler.
  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

#ifdef BOARD_PCA10056
static void buttons_leds_event_handler(bsp_event_t event) {
  ret_code_t err_code;
  switch (event) {
    case BSP_EVENT_DISCONNECT:
      err_code = sd_ble_gap_disconnect(ble::connection::get_handle(),
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      if (err_code != NRF_ERROR_INVALID_STATE) { APP_ERROR_CHECK(err_code); }
      break;

    default:
      break;
  }
}

static void buttons_leds_init() {
  ret_code_t  err_code;
  bsp_event_t startup_event;

  err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, buttons_leds_event_handler);
  APP_ERROR_CHECK(err_code);

  err_code = bsp_btn_ble_init(NULL, &startup_event);
  APP_ERROR_CHECK(err_code);
}
#endif

static void log_init(void) {
  const auto err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void gap_params_init(void) {
  ret_code_t              err_code;
  ble_gap_conn_params_t   gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  sec_mode.sm = 1;
  sec_mode.lv = 3;
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code =
      sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
  APP_ERROR_CHECK(err_code);

  err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency     = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);

  static ble_opt_t m_static_pin_option;

  static uint8_t passkey[] = "123455";

  m_static_pin_option.gap_opt.passkey.p_passkey = &passkey[0];

  err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &m_static_pin_option);

  APP_ERROR_CHECK(err_code);
}

static void gatt_init(void) {
  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
  APP_ERROR_CHECK(err_code);
}

// TODO(khoi): Remove this after development is done
// #pragma GCC diagnostic ignored "-Wunused-function"

// static void reset() { pm::delete_all_bonds_unsafe(); }

/**@brief Function for application main entry.
 */
int main(void) {
  // Initialize.
  log_init();
  timers_init();
#ifdef BOARD_PCA10056
  buttons_leds_init();
#endif
  power::init();

  ble::init();
  gap_params_init();
  gatt_init();
  ble::advertising::init();

  ble::qwr::init();
  ble::bms::init();

  ble::connection::init();
  ble::pm::init();

  // Start execution.
  NRF_LOG_INFO("Bond Management example started.");

  ble::advertising::start();

  for (;;) {
    if (!NRF_LOG_PROCESS()) { power::run(); }
  }
}

/**
 * @}
 */
