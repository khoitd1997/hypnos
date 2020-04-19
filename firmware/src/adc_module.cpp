#include "adc_module.hpp"

#include <array>
#include <limits>

#include "app_error.h"

#include "nrf_drv_saadc.h"

#include "bas_module.hpp"

namespace adc {
  namespace {
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS \
  600 /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION                                                            \
  6 /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of \
       conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define ADC_RES_10BIT 1024

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
  ((((ADC_VALUE)*ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

    typedef void (*adc_event_callback_t)(nrf_drv_saadc_evt_t const* p_event,
                                         const uint16_t             result_millivolt);
    std::array<adc_event_callback_t, 1> m_adc_event_callbacks{ble::bas::adc_event_callback};

    nrf_saadc_value_t m_adc_buf[2];

    void adc_event_handler(nrf_drv_saadc_evt_t const* p_event) {
      auto adc_result_millivolt = std::numeric_limits<uint16_t>::max();

      if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
        adc_result_millivolt = ADC_RESULT_IN_MILLI_VOLTS(p_event->data.done.p_buffer[0]);
        const auto err_code  = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);
      }

      for (const auto& cb : m_adc_event_callbacks) { cb(p_event, adc_result_millivolt); }
    }
  }  // namespace

  void init() {
    // TODO(khoi): Check the low power config
    auto err_code = nrf_drv_saadc_init(nullptr, adc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&m_adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&m_adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace adc