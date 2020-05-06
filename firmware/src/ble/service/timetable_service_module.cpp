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

#include "ble_custom_characteristic_value_type.hpp"
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
    typedef uint16_t time_hour_minute_t;

    template <typename T,
              uint16_t characteristic_uuid,
              bool     variable_length,
              bool     is_custom = std::is_base_of_v<BleCustomCharacteristicValueType, T>,
              std::enable_if_t<std::is_arithmetic_v<T> || is_custom, int> = 0>
    class BleCharacteristic {
     private:
      const uint16_t &_service_handle;
      const uint16_t &_conn_handle;
      const uint8_t & _uuid_type;

      void _update_ble_stack_value(uint8_t *buf, const uint16_t len) {
        ble_gatts_value_t gatts_value{
            .len     = len,
            .offset  = 0,
            .p_value = buf,
        };
        const auto err_code =
            sd_ble_gatts_value_set(_conn_handle, characteristc_handles.value_handle, &gatts_value);
        APP_ERROR_CHECK(err_code);
      }

      void _get_ble_stack_value(uint8_t *buf, uint16_t &len) {
        ble_gatts_value_t gatts_value{
            .len     = 0,
            .offset  = 0,
            .p_value = buf,
        };
        const auto err_code =
            sd_ble_gatts_value_get(_conn_handle, characteristc_handles.value_handle, &gatts_value);
        APP_ERROR_CHECK(err_code);
        len = gatts_value.len;
      }

      uint16_t _get_max_size_in_bytes() const {
        if constexpr (std::is_arithmetic_v<T>) {
          return sizeof(T);
        } else if constexpr (is_custom) {
          return value.max_size_in_bytes();
        }
      }

      uint16_t _get_size_in_bytes() const {
        if constexpr (std::is_arithmetic_v<T>) {
          return _get_max_size_in_bytes();
        } else if constexpr (is_custom) {
          return value.size_in_bytes();
        }
      }

     public:
      T                        value;
      ble_gatts_char_handles_t characteristc_handles;

      BleCharacteristic(const uint16_t &service_handle,
                        const uint16_t &conn_handle,
                        const uint8_t & uuid_type)
          : _service_handle{service_handle}, _conn_handle{conn_handle}, _uuid_type{uuid_type} {}

      void init() {
        const ble_gatts_char_md_t char_md{.char_props{
            .read  = 1,
            .write = 1,
        }};

        ble_gatts_attr_md_t attr_md{
            .vlen = (variable_length ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0)),
            .vloc = BLE_GATTS_VLOC_STACK,
        };
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

        const ble_uuid_t ble_uuid{
            .uuid = characteristic_uuid,
            .type = _uuid_type,
        };
        const ble_gatts_attr_t attr_char_value{
            .p_uuid    = &ble_uuid,
            .p_attr_md = &attr_md,
            .init_len  = 0,
            .init_offs = 0,
            .max_len   = _get_max_size_in_bytes(),
        };

        const auto err_code = sd_ble_gatts_characteristic_add(
            _service_handle, &char_md, &attr_char_value, &characteristc_handles);
        APP_ERROR_CHECK(err_code);
      }

      void sync_local_to_ble_stack() {
        uint8_t *  buf = nullptr;
        const auto len = _get_size_in_bytes();

        if constexpr (std::is_arithmetic_v<T>) {
          buf = (uint8_t *)(&value);
        } else if constexpr (is_custom) {
          buf = value.data();
        }

        _update_ble_stack_value(buf, len);
      }

      void sync_ble_stack_to_local() {
        uint8_t    buf[_get_max_size_in_bytes()] = {0};
        const auto len                           = _get_max_size_in_bytes();

        _get_ble_stack_value(buf, len);

        if constexpr (std::is_arithmetic_v<T>) {
          value = *((T *)(buf));
        } else if constexpr (is_custom) {
          value.replace(buf, len);
        }
      }
    };

    void timetable_service_ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context);

// f364adc9-b000-4042-ba50-05ca45bf8abc
#define TIMETABLE_SERVICE_UUID_BASE \
  { 0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x42, 0x40, 0x00, 0xB0, 0xC9, 0xAD, 0x64, 0xF3 }

    constexpr uint16_t TIMETABLE_SERVICE_UUID                          = 0x1400;
    constexpr uint16_t TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID    = 0x1401;
    constexpr uint16_t TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID = 0x1405;

    struct timetable_service_t {
      uint16_t service_handle;

      uint16_t conn_handle;
      uint8_t  uuid_type;

      BleCharacteristic<time_hour_minute_t, TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID, false>
          morning_curfew_characteristic{service_handle, conn_handle, uuid_type};
      BleCharacteristic<TimeExceptionList, TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID, true>
          active_exceptions_characteristic{service_handle, conn_handle, uuid_type};
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

    m_timetable.morning_curfew_characteristic.init();
    m_timetable.morning_curfew_characteristic.value = 5;
    m_timetable.morning_curfew_characteristic.sync_local_to_ble_stack();

    m_timetable.active_exceptions_characteristic.init();
    m_timetable.active_exceptions_characteristic.value.push({5, 10});
    m_timetable.active_exceptions_characteristic.value.push({11, 15});
    m_timetable.active_exceptions_characteristic.sync_local_to_ble_stack();
  }
}  // namespace ble::timetable_service