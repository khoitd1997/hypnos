#include "timetable_service_module.hpp"

#include <cstring>
#include <ctime>

#include <array>

#include "ble.h"
#include "ble_srv_common.h"
#include "boards.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_sdh_ble.h"

#include "sdk_common.h"

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

    void timetable_service_ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context);
    struct timetable_service_t {
      uint16_t service_handle;

      ble_gatts_char_handles_t morning_curfew_handle;
      ble_gatts_char_handles_t active_exceptions_handle;

      uint16_t conn_handle;
      uint8_t  uuid_type;
    };
    TIMETABLE_SERVICE_DEF(m_timetable);

// f364adc9-b000-4042-ba50-05ca45bf8abc
#define TIMETABLE_SERVICE_UUID_BASE \
  { 0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0xC9, 0xAD, 0x64, 0xF3 }

    constexpr uint16_t TIMETABLE_SERVICE_UUID                          = 0x1400;
    constexpr uint16_t TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID    = 0x1401;
    constexpr uint16_t TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID = 0x1405;

    class ExceptionList {
     private:
      static constexpr size_t MAX_ACTIVE_EXCEPTIONS = 4;
      static constexpr size_t SIZE_OF_ONE_EXCEPTION = sizeof(time_t) * 2;

      std::array<time_t, MAX_ACTIVE_EXCEPTIONS * 2> _raw_times;
      size_t                                        _total_exception = 0;

     public:
      uint16_t size_in_bytes() const {
        return static_cast<uint16_t>(SIZE_OF_ONE_EXCEPTION * _total_exception);
      }
      uint16_t max_size_in_bytes() const {
        return static_cast<uint16_t>(SIZE_OF_ONE_EXCEPTION * MAX_ACTIVE_EXCEPTIONS);
      }
      size_t size() const { return _total_exception; }

      bool is_full() const { return _total_exception == MAX_ACTIVE_EXCEPTIONS; }
      bool is_empty() const { return _total_exception == 0; }
      void set_empty() { _total_exception = 0; }

      uint8_t *data() const { return (uint8_t *)(const_cast<time_t *>(_raw_times.data())); }

      bool push(const time_t &start_time, const time_t &end_time) {
        if (is_full()) { return false; }

        _raw_times.at(_total_exception * 2)     = start_time;
        _raw_times.at(_total_exception * 2 + 1) = end_time;
        ++_total_exception;
        return true;
      }
      bool get(const size_t index, time_t &start_time, time_t &end_time) const {
        if (index >= _total_exception) { return false; }

        start_time = _raw_times.at(index * 2);
        end_time   = _raw_times.at(index * 2 + 1);
        return true;
      }
    };
    ExceptionList m_active_exceptions;

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
          ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

          if (p_evt_write->handle == m_timetable.morning_curfew_handle.value_handle) {
            nrf_gpio_pin_toggle(LED_4);
            /*
            if(*p_evt_write->data == 0x01)
            {
                nrf_gpio_pin_clear(20);
            }
            else if(*p_evt_write->data == 0x02)
            {
                nrf_gpio_pin_set(20);
            }
            else
            {
              //Do nothing
            }
            */
          }
        } break;

        default:
          // No implementation needed.
          break;
      }
    }

    void morning_curfew_characteristic_add() {
      const ble_gatts_char_md_t char_md{.char_props{
          .read  = 1,
          .write = 1,
      }};

      ble_gatts_attr_md_t attr_md{.vloc = BLE_GATTS_VLOC_STACK};
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

      const ble_uuid_t ble_uuid{
          .uuid = TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID,
          .type = m_timetable.uuid_type,
      };
      const ble_gatts_attr_t attr_char_value{
          .p_uuid    = &ble_uuid,
          .p_attr_md = &attr_md,
          .init_len  = sizeof(time_hour_minute_t),
          .init_offs = 0,
          .max_len   = sizeof(time_hour_minute_t),
      };

      const auto err_code = sd_ble_gatts_characteristic_add(m_timetable.service_handle,
                                                            &char_md,
                                                            &attr_char_value,
                                                            &m_timetable.morning_curfew_handle);
      APP_ERROR_CHECK(err_code);
    }

    void active_exceptions_characteristic_add() {
      const ble_gatts_char_md_t char_md{.char_props{
          .read  = 1,
          .write = 1,
      }};

      ble_gatts_attr_md_t attr_md{
          .vlen = 1,
          .vloc = BLE_GATTS_VLOC_STACK,
      };
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

      const ble_uuid_t ble_uuid{
          .uuid = TIMETABLE_ACTIVE_EXCEPTIONS_CHARACTERISTIC_UUID,
          .type = m_timetable.uuid_type,
      };
      const ble_gatts_attr_t attr_char_value{
          .p_uuid    = &ble_uuid,
          .p_attr_md = &attr_md,
          .init_len  = 0,  // TODO(khoi): Make sure this is variable
          .init_offs = 0,
          .max_len   = m_active_exceptions.max_size_in_bytes(),
      };

      const auto err_code = sd_ble_gatts_characteristic_add(m_timetable.service_handle,
                                                            &char_md,
                                                            &attr_char_value,
                                                            &m_timetable.active_exceptions_handle);
      APP_ERROR_CHECK(err_code);
    }

    // void value_update(uint8_t val) {
    //   NRF_LOG_INFO("updating timetable service value");

    //   ble_gatts_value_t gatts_value{
    //       .len     = sizeof(val),
    //       .offset  = 0,
    //       .p_value = &val,
    //   };
    //   const auto err_code = sd_ble_gatts_value_set(
    //       m_timetable.conn_handle, m_timetable.morning_curfew_handle.value_handle, &gatts_value);
    //   APP_ERROR_CHECK(err_code);
    // }
  }  // namespace

  void value_update() {
    NRF_LOG_INFO("updating timetable service value");

    m_active_exceptions.push(5, 10);
    m_active_exceptions.push(11, 15);

    ble_gatts_value_t gatts_value{
        .len     = m_active_exceptions.size_in_bytes(),
        .offset  = 0,
        .p_value = m_active_exceptions.data(),
    };
    const auto err_code = sd_ble_gatts_value_set(
        m_timetable.conn_handle, m_timetable.active_exceptions_handle.value_handle, &gatts_value);
    APP_ERROR_CHECK(err_code);
  }

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

    morning_curfew_characteristic_add();
    active_exceptions_characteristic_add();
    value_update();
  }
}  // namespace ble::timetable_service