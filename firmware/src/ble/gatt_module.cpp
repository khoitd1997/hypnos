#include "gatt_module.hpp"

#include "app_error.h"

#include "nrf_ble_gatt.h"

#include "nrf_sdh_ble.h"

#include "nrf_log.h"

namespace ble::gatt {
  namespace {
    NRF_BLE_GATT_DEF(m_gatt);

    void gatt_event_handler(nrf_ble_gatt_t* p_gatt, nrf_ble_gatt_evt_t const* p_evt) {
      switch (p_evt->evt_id) {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED: {
          NRF_LOG_INFO("ATT MTU exchange completed. MTU set to %u bytes.",
                       p_evt->params.att_mtu_effective);

          const auto err_code = nrf_ble_gatt_data_length_set(
              &m_gatt, BLE_CONN_HANDLE_INVALID, NRF_SDH_BLE_GAP_DATA_LENGTH);
          APP_ERROR_CHECK(err_code);
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED: {
          NRF_LOG_INFO("Data length updated to %u bytes.", p_evt->params.data_length);
        } break;
      }
    }
  }  // namespace

  void init() {
    auto err_code = nrf_ble_gatt_init(&m_gatt, gatt_event_handler);
    APP_ERROR_CHECK(err_code);

    // doesn't seem to work on the Oneplus 6T, since they seem to require symmetric RX/TX for data
    // length extension
    err_code =
        nrf_ble_gatt_data_length_set(&m_gatt, BLE_CONN_HANDLE_INVALID, NRF_SDH_BLE_GAP_DATA_LENGTH);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::gatt