#include "timetable_service_module.hpp"

#include <cstring>

#include <array>
#include <functional>
#include <type_traits>

#include "ble.h"
#include "ble_srv_common.h"
#include "boards.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_sdh_ble.h"

#include "sdk_common.h"

#include "pm_module.hpp"

#include "ble_characteristic.hpp"
#include "time_exception_list.hpp"

#include "rv3028.hpp"

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

    constexpr uint16_t TIMETABLE_CURRENT_UNIX_TIME_CHARACTERISTIC_UUID = 0x1407;

    struct timetable_service_t {
      uint16_t service_handle;

      uint16_t conn_handle;
      uint8_t  uuid_type;

      BleCharacteristic<TimeHourMinute> morning_curfew_characteristic{
          service_handle, conn_handle, uuid_type, TIMETABLE_MORNING_CURFEW_CHARACTERISTIC_UUID};
      BleCharacteristic<TimeHourMinute> night_curfew_characteristic{
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

      BleCharacteristic<unix_time_t> current_unix_time_characteristic{
          service_handle, conn_handle, uuid_type, TIMETABLE_CURRENT_UNIX_TIME_CHARACTERISTIC_UUID};
    };
    TIMETABLE_SERVICE_DEF(m_timetable);

    void timetable_service_ble_event_handler(ble_evt_t const *p_ble_evt, void *p_context) {
      //   NRF_LOG_INFO("timetable service event received. Event type = %d",
      //   p_ble_evt->header.evt_id);

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
          NRF_LOG_INFO("gatt write");
          ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

          if (p_evt_write->handle ==
              p_timetable->current_unix_time_characteristic.characteristc_handles.value_handle) {
            NRF_LOG_INFO("current time write");
            RV3028::get().setUNIX(p_timetable->current_unix_time_characteristic.get());
            NRF_LOG_INFO("unix time: %u", RV3028::get().getUNIX());
          }
        } break;

        default:
          // No implementation needed.
          break;
      }
    }
  }  // namespace

  BleCharacteristic<TimeHourMinute> &morning_curfew_characteristic =
      m_timetable.morning_curfew_characteristic;
  BleCharacteristic<TimeHourMinute> &night_curfew_characteristic =
      m_timetable.night_curfew_characteristic;

  BleCharacteristic<uint8_t> &work_duration_characteristic =
      m_timetable.work_duration_characteristic;
  BleCharacteristic<uint8_t> &break_duration_characteristic =
      m_timetable.break_duration_characteristic;

  BleCharacteristic<TimeExceptionList> &active_exceptions_characteristic =
      m_timetable.active_exceptions_characteristic;

  BleCharacteristic<uint8_t> &tokens_left_characteristic = m_timetable.tokens_left_characteristic;

  BleCharacteristic<unix_time_t> &current_unix_time_characteristic =
      m_timetable.current_unix_time_characteristic;

  class StorageManager {
    void _copy_bytes_and_advance_pointer(const uint8_t *source,
                                         uint8_t **     destination,
                                         const uint8_t  size,
                                         const uint8_t  size_to_advance) {
      for (auto i = 0; i < size; ++i) {
        *(*destination) = source[i];
        *destination += sizeof(uint8_t);
      }

      *destination += (size_to_advance - size);
    }

    template <typename T>
    uint8_t _get_size_in_bytes_to_store_value(const BleCharacteristic<T> &value) {
      return value.get_max_size_in_bytes() + (value.is_variable_length ? sizeof(uint8_t) : 0);
    }

    template <typename T, typename... Args>
    uint8_t _get_size_in_bytes_to_store_value(const BleCharacteristic<T> &value, Args &&... args) {
      return _get_size_in_bytes_to_store_value(value) +
             _get_size_in_bytes_to_store_value(std::forward<Args>(args)...);
    }

    template <typename T>
    void _serialize(uint8_t **destination, const BleCharacteristic<T> &value) {
      if (value.is_variable_length) {
        const auto len = value.get_size_in_bytes();
        _copy_bytes_and_advance_pointer(&len, destination, sizeof(len), sizeof(len));
      }
      _copy_bytes_and_advance_pointer(
          value.data(), destination, value.get_size_in_bytes(), value.get_max_size_in_bytes());
    }
    template <typename T, typename... Args>
    void _serialize(uint8_t **destination, const BleCharacteristic<T> &value, Args &&... args) {
      _serialize(destination, value);
      _serialize(destination, std::forward<Args>(args)...);
    }

    template <typename T>
    void _deserialize(uint8_t **source, BleCharacteristic<T> &value) {
      auto len = value.get_max_size_in_bytes();
      if (value.is_variable_length) {
        len = *(*source);
        *source += sizeof(len);
      }
      value.set(*source, len);
      *source += value.get_max_size_in_bytes();
    }
    template <typename T, typename... Args>
    void _deserialize(uint8_t **source, BleCharacteristic<T> &value, Args &&... args) {
      _deserialize(source, value);
      _deserialize(source, std::forward<Args>(args)...);
    }

   public:
    template <typename... Args>
    void store(Args &&... args) {
      const auto size      = _get_size_in_bytes_to_store_value(std::forward<Args>(args)...);
      auto &     rtc       = RV3028::get();
      uint8_t    buf[size] = {0};
      uint8_t *  buf_ptr   = &(buf[0]);
      _serialize(&buf_ptr, std::forward<Args>(args)...);
      rtc.writeUserEEPROM(buf, size);
    }

    template <typename... Args>
    void restore(Args &&... args) {
      const auto size      = _get_size_in_bytes_to_store_value(std::forward<Args>(args)...);
      uint8_t    buf[size] = {0};

      auto &rtc = RV3028::get();
      rtc.readUserEEPROM(buf, size);
      auto buf_ptr = &(buf[0]);
      _deserialize(&buf_ptr, std::forward<Args>(args)...);
    }
  };
  StorageManager storage_manager;

// clang-format off
#define CHARACTERISTICS_TO_STORE \
    morning_curfew_characteristic, night_curfew_characteristic, \
    work_duration_characteristic, break_duration_characteristic, \
    active_exceptions_characteristic, \
    tokens_left_characteristic
  // clang-format on

  void test_storage_manager() {
    NRF_LOG_INFO("Starting storage manager test");

    StorageManager test_storage_manager;

    const auto morning_curfew = TimeHourMinute{2, 30};
    const auto night_curfew   = TimeHourMinute{23, 45};

    uint8_t work_duration  = 20;
    uint8_t break_duration = 50;

    TimeExceptionList exception_list;
    exception_list.push(TimeException{2222, 3333});
    exception_list.push(TimeException{40000, 90000});

    uint8_t tokens_left = 44;

    morning_curfew_characteristic.set(morning_curfew);
    night_curfew_characteristic.set(night_curfew);

    work_duration_characteristic.set(work_duration);
    break_duration_characteristic.set(break_duration);

    active_exceptions_characteristic.set(exception_list);

    tokens_left_characteristic.set(tokens_left);

    test_storage_manager.store(CHARACTERISTICS_TO_STORE);

    {
      morning_curfew_characteristic.set(TimeHourMinute{5, 50});
      night_curfew_characteristic.set(TimeHourMinute{22, 3});

      work_duration_characteristic.set(35);
      break_duration_characteristic.set(32);

      TimeExceptionList temp_list;
      temp_list.push(TimeException{1111, 55555});
      temp_list.push(TimeException{44, 90});
      active_exceptions_characteristic.set(temp_list);

      tokens_left_characteristic.set(11);
    }

    test_storage_manager.restore(CHARACTERISTICS_TO_STORE);

    ASSERT(morning_curfew_characteristic.get() == morning_curfew);
    ASSERT(night_curfew_characteristic.get() == night_curfew);

    ASSERT(work_duration_characteristic.get() == work_duration);
    ASSERT(break_duration_characteristic.get() == break_duration);

    ASSERT(active_exceptions_characteristic.get() == exception_list);

    ASSERT(tokens_left_characteristic.get() == tokens_left);

    NRF_LOG_INFO("Ending storage manager test");
    NRF_LOG_FLUSH();
  }

  void load_test_data() {
    const auto morning_curfew = TimeHourMinute{2, 30};
    const auto night_curfew   = TimeHourMinute{23, 45};

    uint8_t work_duration  = 20;
    uint8_t break_duration = 50;

    TimeExceptionList exception_list;
    exception_list.push(TimeException{2222, 3333});
    exception_list.push(TimeException{40000, 90000});

    uint8_t tokens_left = 44;

    morning_curfew_characteristic.set(morning_curfew);
    night_curfew_characteristic.set(night_curfew);

    work_duration_characteristic.set(work_duration);
    break_duration_characteristic.set(break_duration);

    active_exceptions_characteristic.set(exception_list);

    tokens_left_characteristic.set(tokens_left);
  }

  void init() {
    NRF_LOG_INFO("Initting Timetable Characteristic");
    NRF_LOG_FLUSH();
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
    night_curfew_characteristic.init();

    work_duration_characteristic.init();
    break_duration_characteristic.init();

    active_exceptions_characteristic.init();

    tokens_left_characteristic.init();

    current_unix_time_characteristic.init();

    if (pm::is_bonded()) {
      NRF_LOG_INFO("Restoring timetable characteristics");
      storage_manager.restore(CHARACTERISTICS_TO_STORE);
    }
  }

  void store_data_to_eeprom() { storage_manager.store(CHARACTERISTICS_TO_STORE); }
}  // namespace ble::timetable_service