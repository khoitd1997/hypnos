#include "adc_module.hpp"

#include <algorithm>

#include "app_error.h"

#include "nrf_drv_saadc.h"

#include "nrf_log.h"

#include "bas_module.hpp"

namespace adc {
  namespace {
    constexpr int16_t ADC_REF_VOLTAGE_IN_MILLIVOLT = 600;
    constexpr int16_t ADC_PRE_SCALING_COMPENSATION = 6;
    constexpr int16_t ADC_RES_10BIT                = 1024;

    inline int16_t conv_adc_result_to_millivolt(const nrf_saadc_value_t val) {
      return ((val * ADC_REF_VOLTAGE_IN_MILLIVOLT) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION;
    }

    constexpr uint8_t  ADC_VDD_CHANNEL               = 0;
    constexpr uint16_t DIODE_FWD_VOLT_DROP_MILLIVOLT = 270;

    void adc_event_handler(nrf_drv_saadc_evt_t const* p_event) {}
  }  // namespace

  void init() {
    nrf_saadc_channel_config_t config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);

    auto err_code = nrf_drv_saadc_init(nullptr, adc_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(ADC_VDD_CHANNEL, &config);
    APP_ERROR_CHECK(err_code);
  }

  int16_t sample_in_millivolt(const uint8_t channel) {
    nrf_saadc_value_t temp;
    const auto        err_code = nrf_drv_saadc_sample_convert(channel, &temp);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("ADC mv: %d", conv_adc_result_to_millivolt(temp));

    return conv_adc_result_to_millivolt(temp);
  }

  uint16_t sample_battery_in_millivolt() {
    return std::max(DIODE_FWD_VOLT_DROP_MILLIVOLT + sample_in_millivolt(ADC_VDD_CHANNEL), 0);
  }
}  // namespace adc