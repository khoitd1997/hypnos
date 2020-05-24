#ifndef _BLE_CHARACTERISTIC_HPP
#define _BLE_CHARACTERISTIC_HPP

#include <cstdint>

#include <type_traits>

#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#include "app_error.h"

#include "ble_custom_characteristic_value_type.hpp"

/**
 * @brief Helper class used for creating ble characteristics in custom services
 *
 */
template <typename T,
          bool is_custom = std::is_base_of_v<BleCustomCharacteristicValueType, T>,
          std::enable_if_t<std::is_arithmetic_v<T> || is_custom, int> = 0>
class BleCharacteristic {
 private:
  const uint16_t &_service_handle;
  const uint16_t &_conn_handle;
  const uint8_t & _uuid_type;
  const uint16_t  _uuid;

  T _value;

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

  void _get_ble_stack_value(uint8_t *buf, const uint16_t max_len, uint16_t &curr_len) {
    ble_gatts_value_t gatts_value{
        .len     = max_len,
        .offset  = 0,
        .p_value = buf,
    };
    const auto err_code =
        sd_ble_gatts_value_get(_conn_handle, characteristc_handles.value_handle, &gatts_value);
    APP_ERROR_CHECK(err_code);
    curr_len = gatts_value.len;
  }

 public:
  ble_gatts_char_handles_t characteristc_handles;
  const bool               is_variable_length;

  BleCharacteristic(const uint16_t &service_handle,
                    const uint16_t &conn_handle,
                    const uint8_t & uuid_type,
                    const uint16_t  uuid,
                    const bool      variable_length = false)
      : _service_handle{service_handle},
        _conn_handle{conn_handle},
        _uuid_type{uuid_type},
        _uuid{uuid},
        is_variable_length{variable_length} {}

  void init() {
    const ble_gatts_char_md_t char_md{.char_props{
        .read  = 1,
        .write = 1,
    }};

    ble_gatts_attr_md_t attr_md{
        .vlen = (is_variable_length ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0)),
        .vloc = BLE_GATTS_VLOC_STACK,
    };
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    const ble_uuid_t ble_uuid{
        .uuid = _uuid,
        .type = _uuid_type,
    };
    const ble_gatts_attr_t attr_char_value{
        .p_uuid    = &ble_uuid,
        .p_attr_md = &attr_md,
        .init_len  = 0,
        .init_offs = 0,
        .max_len   = get_max_size_in_bytes(),
    };

    const auto err_code = sd_ble_gatts_characteristic_add(
        _service_handle, &char_md, &attr_char_value, &characteristc_handles);
    APP_ERROR_CHECK(err_code);
  }

  uint8_t get_max_size_in_bytes() const {
    if constexpr (std::is_arithmetic_v<T>) {
      return sizeof(_value);
    } else if constexpr (is_custom) {
      return _value.max_size_in_bytes();
    }
  }

  uint8_t get_size_in_bytes() const {
    if constexpr (std::is_arithmetic_v<T>) {
      return get_max_size_in_bytes();
    } else if constexpr (is_custom) {
      return _value.size_in_bytes();
    }
  }

  uint8_t *data() const {
    if constexpr (std::is_arithmetic_v<T>) {
      return (uint8_t *)(&_value);
    } else if constexpr (is_custom) {
      return _value.data();
    }
  }

  void set(const T &value) {
    _value = value;

    uint8_t *  buf = data();
    const auto len = get_size_in_bytes();

    set(buf, len);
  }

  void set(uint8_t *buf, const uint16_t len) {
    _update_ble_stack_value(buf, len);
    get();
  }

  T get() {
    uint8_t    buf[get_max_size_in_bytes()] = {0};
    const auto max_len                      = get_max_size_in_bytes();
    uint16_t   curr_len                     = 0;

    _get_ble_stack_value(buf, max_len, curr_len);

    if constexpr (std::is_arithmetic_v<T>) {
      _value = *((T *)(buf));
    } else if constexpr (is_custom) {
      ASSERT(_value.replace(buf, curr_len));
    }

    return _value;
  }
};

#endif