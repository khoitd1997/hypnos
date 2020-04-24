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
  //!< Value used as error code on stack dump, can be used to identify stack location on
  //!< stack unwind.
  app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

// TODO(khoi): Remove this after development is done
// #pragma GCC diagnostic ignored "-Wunused-function"

// static void reset() { pm::delete_all_bonds_unsafe(); }

BLE_CUS_DEF(m_cus);

static void on_cus_evt(ble_cus_t* p_cus_service, ble_cus_evt_t* p_evt) {
  switch (p_evt->evt_type) {
    case BLE_CUS_EVT_CONNECTED:
      break;

    case BLE_CUS_EVT_DISCONNECTED:
      break;

    default:
      // No implementation needed.
      break;
  }
}

APP_TIMER_DEF(m_timer_id);

/**@brief Function for application main entry.
 */
int main(void) {
  misc::log::init();
  misc::timer::init();
  misc::bsp::init();
  power::init();

  ble::init();
  adc::init();
  ble::gap::init();
  ble::gatt::init();

  ble::qwr::init();

  ble_cus_init_t cus_init = {0};
  // Initialize CUS Service init structure to zero.
  cus_init.evt_handler = on_cus_evt;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init.custom_value_char_attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init.custom_value_char_attr_md.write_perm);

  auto err_code = ble_cus_init(&m_cus, &cus_init);
  APP_ERROR_CHECK(err_code);

  ble::bms::init();
  ble::bas::init();

  ble::advertising::init();

  ble::connection::init();
  ble::pm::init();

  //   misc::timer::create(APP_TIMER_MODE_REPEATED, &m_timer_id, [](void* ctx) { ble::bas::update();
  //   }); app_timer_start(m_timer_id, APP_TIMER_TICKS(6000), nullptr);

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
