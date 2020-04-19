#include "adc_module.hpp"

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

    constexpr uint8_t ADC_DEFAULT_CHANNEL = 0;

    void adc_event_handler(nrf_drv_saadc_evt_t const* p_event) {}
  }  // namespace

  void init() {
    // TODO(khoi): Check the low power config
    auto err_code = nrf_drv_saadc_init(nullptr, adc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(SAADC_CH_PSELP_PSELP_AnalogInput0);
    err_code = nrf_drv_saadc_channel_init(ADC_DEFAULT_CHANNEL, &config);
    APP_ERROR_CHECK(err_code);
  }

  uint16_t sample_in_millivolt() {
    nrf_saadc_value_t temp;
    const auto        err_code = nrf_drv_saadc_sample_convert(ADC_DEFAULT_CHANNEL, &temp);
    APP_ERROR_CHECK(err_code);

    return ADC_RESULT_IN_MILLI_VOLTS(temp);
  }
}  // namespace adc