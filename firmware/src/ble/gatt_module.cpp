#include "gatt_module.hpp"

#include "app_error.h"

#include "nrf_ble_gatt.h"

#include "nrf_sdh_ble.h"

namespace ble::gatt {
  namespace {
    NRF_BLE_GATT_DEF(m_gatt);
  }

  void init() {
    const auto err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::gatt