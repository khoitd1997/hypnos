#include "timetable_service_module.hpp"

#include <cstring>

#include <array>
#include <type_traits>

#include "ble.h"
#include "ble_srv_common.h"
#include "boards.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_sdh_ble.h"

#include "sdk_common.h"

#include "ble_characteristic.hpp"
#include "time_exception_list.hpp"

// clang-format off
#define TIMETABLE_SERVICE_DEF(_name)                          \
    static timetable_service_t _name;                         \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,             \
                         TIMETABLE_SERVICE_OBSERVER_PRIO, \
                         timetable_service_ble_event_handler,        \
                         &_name)
// clang-format on

namespace ble::timetable_service {
  namespace {
    void timetable_service_ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context);

// f364adc9-b000-4042-ba50-05ca45bf8abc
#define TIMETABLE_SERVICE_UUID_BASE \
  { 0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x42, 0x40, 0x00, 0xB0, 0xC9, 0xAD, 0x64, 0xF3 }

    constexpr uint16_t TIMETABLE_SERVICE_UUID = 0x1400;

    constexpr uint16_t TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID = 0x1401;
    constexpr uint16_t TIMETABLE_NIGHT_CURFEW_CHARACTERISTIC_UUID   = 0x1402;

    constexpr uint16_t TIMETABLE_WORK_DURATION_MINUTE_CHARACTERISTIC_UUID  = 0x1403;
    constexpr uint16_t TIMETABLE_BREAK_DURATION_MINUTE_CHARACTERISTIC_UUID = 0x1404;

    constexpr uint16_t TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID = 0x1405;

    constexpr uint16_t TIMETABLE_TOKENS_LEFT_CHARACTERISTIC_UUID = 0x1406;

    struct timetable_service_t {
      uint16_t service_handle;

      uint16_t conn_handle;
      uint8_t  uuid_type;

      BleCharacteristic<time_hour_minute_t> morning_curfew_characteristic{
          service_handle, conn_handle, uuid_type, TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID};
      BleCharacteristic<time_hour_minute_t> night_curfew_characteristic{
          service_handle, conn_handle, uuid_type, TIMETABLE_NIGHT_CURFEW_CHARACTERISTIC_UUID};

      BleCharacteristic<uint8_t> work_duration_characteristic{
          service_handle,
          conn_handle,
          uuid_type,
          TIMETABLE_WORK_DURATION_MINUTE_CHARACTERISTIC_UUID};
      BleCharacteristic<uint8_t> break_duration_characteristic{
          service_handle,
          conn_handle,
          uuid_type,
          TIMETABLE_BREAK_DURATION_MINUTE_CHARACTERISTIC_UUID};

      BleCharacteristic<TimeExceptionList> active_exceptions_characteristic{
          service_handle,
          conn_handle,
          uuid_type,
          TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID,
          true};

      BleCharacteristic<uint8_t> tokens_left_characteristic{
          service_handle, conn_handle, uuid_type, TIMETABLE_TOKENS_LEFT_CHARACTERISTIC_UUID};
    };
    TIMETABLE_SERVICE_DEF(m_timetable);

    void timetable_service_ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context) {
      NRF_LOG_INFO("timetable service event received. Event type = %d", p_ble_evt->header.evt_id);

      auto p_timetable = static_cast<timetable_service_t *>(p_context);

      if (p_timetable == NULL || p_ble_evt == NULL) { return; }

      switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
          m_timetable.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
          break;

        case BLE_GAP_EVT_DISCONNECTED:
          m_timetable.conn_handle = BLE_CONN_HANDLE_INVALID;
          break;

        case BLE_GATTS_EVT_WRITE: {
          //   NRF_LOG_INFO("gatt write");
          //   ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

          //   if (p_evt_write->handle == m_timetable.morning_curfew_handle.value_handle) {
          //     nrf_gpio_pin_toggle(LED_4);
          //     NRF_LOG_INFO("morning curfew value");
          //   // NRF_LOG_INFO("morning curfew value: %u",
          //   //              m_timetable.morning_curfew_characteristic.value);
          //   /*
          //   }
        } break;

        default:
          // No implementation needed.
          break;
      }
    }
  }  // namespace

  BleCharacteristic<time_hour_minute_t> &morning_curfew_characteristic =
      m_timetable.morning_curfew_characteristic;
  BleCharacteristic<time_hour_minute_t> &night_curfew_characteristic =
      m_timetable.night_curfew_characteristic;

  BleCharacteristic<uint8_t> &work_duration_characteristic =
      m_timetable.work_duration_characteristic;
  BleCharacteristic<uint8_t> &break_duration_characteristic =
      m_timetable.break_duration_characteristic;

  BleCharacteristic<TimeExceptionList> &active_exceptions_characteristic =
      m_timetable.active_exceptions_characteristic;

  BleCharacteristic<uint8_t> &tokens_left_characteristic = m_timetable.tokens_left_characteristic;

  void init() {
    m_timetable.conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_uuid128_t base_uuid{TIMETABLE_SERVICE_UUID_BASE};
    auto          err_code = sd_ble_uuid_vs_add(&base_uuid, &m_timetable.uuid_type);
    APP_ERROR_CHECK(err_code);

    const ble_uuid_t ble_uuid{
        .uuid = TIMETABLE_SERVICE_UUID,
        .type = m_timetable.uuid_type,
    };
    err_code = sd_ble_gatts_service_add(
        BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_timetable.service_handle);
    APP_ERROR_CHECK(err_code);

    morning_curfew_characteristic.init();
    morning_curfew_characteristic.set(5);

    night_curfew_characteristic.init();

    work_duration_characteristic.init();
    break_duration_characteristic.init();

    active_exceptions_characteristic.init();
    TimeExceptionList exception_list;
    exception_list.push({866450344, 902749805});
    exception_list.push({1015322584, 1537641659});
    active_exceptions_characteristic.set(exception_list);

    tokens_left_characteristic.init();
  }
}  // namespace ble::timetable_service