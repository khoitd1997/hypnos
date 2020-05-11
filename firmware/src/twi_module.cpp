#include "twi_module.hpp"

#include "ble.h"

#include "nrf_drv_twi.h"

#include "app_error.h"

#include "power_module.hpp"

namespace twi {
  namespace {
    // volatile auto m_xfer_done = false;

    constexpr uint32_t TWI_SCL_PIN = 27;
    constexpr uint32_t TWI_SDA_PIN = 26;

    const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);

    // #define LM75B_ADDR (0x90U >> 1)

    void twi_handler([[maybe_unused]] nrf_drv_twi_evt_t const* p_event,
                     [[maybe_unused]] void*                    p_context) {
      //   switch (p_event->type) {
      //     case NRF_DRV_TWI_EVT_DONE:
      //       if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX) { data_handler(m_sample); }
      //       m_xfer_done = true;
      //       break;
      //     default:
      //       break;
      //   }
    }

    void wait_till_bus_free() {
      while (nrf_drv_twi_is_busy(&m_twi)) { power::run(); }
    }
  }  // namespace

  void init() {
    constexpr nrf_drv_twi_config_t twi_config{.scl                = TWI_SCL_PIN,
                                              .sda                = TWI_SDA_PIN,
                                              .frequency          = NRF_DRV_TWI_FREQ_100K,
                                              .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
                                              .clear_bus_init     = false};

    const auto err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, nullptr);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
  }

  void read(const uint8_t address, uint8_t* p_data, const uint8_t length) {
    // m_xfer_done = false;
    wait_till_bus_free();

    const auto err_code = nrf_drv_twi_rx(&m_twi, address, p_data, length);
    APP_ERROR_CHECK(err_code);

    wait_till_bus_free();
  }
  void write(const uint8_t  address,
             uint8_t const* p_data,
             const uint8_t  length,
             const bool     no_stop) {
    const auto err_code = nrf_drv_twi_tx(&m_twi, address, p_data, length, no_stop);
    APP_ERROR_CHECK(err_code);
    wait_till_bus_free();
  }
}  // namespace twi