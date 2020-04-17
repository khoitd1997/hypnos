#include "qwr_module.hpp"

#include "app_error.h"

#include "nrf_sdh_ble.h"

#include "bms_module.hpp"

namespace ble::qwr {
  namespace {
    constexpr size_t QWR_MEM_BUFF_SIZE = 512;
    uint8_t          m_qwr_mem[QWR_MEM_BUFF_SIZE];

    NRF_BLE_QWR_DEF(m_qwr);

    uint16_t qwr_event_handler(nrf_ble_qwr_t *p_qwr, nrf_ble_qwr_evt_t *p_evt) {
      return bms::qwr_event_callback(p_qwr, p_evt);
    }

    void qwr_error_handler(uint32_t nrf_error) { APP_ERROR_HANDLER(nrf_error); }
  }  // namespace

  void init() {
    nrf_ble_qwr_init_t qwr_init{};

    qwr_init.mem_buffer.len   = QWR_MEM_BUFF_SIZE;
    qwr_init.mem_buffer.p_mem = m_qwr_mem;
    qwr_init.callback         = qwr_event_handler;
    qwr_init.error_handler    = qwr_error_handler;

    const auto err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);
  }

  void conn_handle_set(uint16_t conn_handle) {
    const auto err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, conn_handle);
    APP_ERROR_CHECK(err_code);
  }

  nrf_ble_qwr_t *get() { return &m_qwr; }
}  // namespace ble::qwr