#include "timetable_service_module.hpp"

#include <cstring>

#include "ble.h"
#include "ble_srv_common.h"
#include "boards.h"

#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_sdh_ble.h"

#include "sdk_common.h"

struct ble_cus_t {
  uint16_t                 service_handle;
  ble_gatts_char_handles_t custom_value_handles;
  uint16_t                 conn_handle;
  uint8_t                  uuid_type;
};

// clang-format off
#define BLE_CUS_DEF(_name)                          \
    static ble_cus_t _name;                         \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,             \
                         TIMETABLE_SERVICE_OBSERVER_PRIO, \
                         ble_cus_on_ble_evt,        \
                         &_name)
// clang-format on

namespace ble::timetable_service {
  namespace {
    void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);

    BLE_CUS_DEF(m_cus);

    // f364adc9-b000-4042-ba50-05ca45bf8abc
#define CUSTOM_SERVICE_UUID_BASE \
  { 0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0xC9, 0xAD, 0x64, 0xF3 }

#define CUSTOM_SERVICE_UUID 0x1400
#define CUSTOM_VALUE_CHAR_UUID 0x1401

    void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
      NRF_LOG_INFO("timetable service event received. Event type = %d", p_ble_evt->header.evt_id);

      auto p_cus = static_cast<ble_cus_t *>(p_context);

      if (p_cus == NULL || p_ble_evt == NULL) { return; }

      switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
          m_cus.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
          break;

        case BLE_GAP_EVT_DISCONNECTED:
          m_cus.conn_handle = BLE_CONN_HANDLE_INVALID;
          break;

        case BLE_GATTS_EVT_WRITE: {
          ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

          // Custom Value Characteristic Written to.
          if (p_evt_write->handle == m_cus.custom_value_handles.value_handle) {
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

    void custom_value_char_add() {
      const ble_gatts_char_md_t char_md{.char_props{
          .read  = 1,
          .write = 1,
      }};

      ble_gatts_attr_md_t attr_md{.vloc = BLE_GATTS_VLOC_STACK};
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
      BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

      const ble_uuid_t ble_uuid{
          .uuid = CUSTOM_VALUE_CHAR_UUID,
          .type = m_cus.uuid_type,
      };
      const ble_gatts_attr_t attr_char_value{
          .p_uuid    = &ble_uuid,
          .p_attr_md = &attr_md,
          .init_len  = sizeof(uint8_t),
          .init_offs = 0,
          .max_len   = sizeof(uint8_t),
      };

      const auto err_code = sd_ble_gatts_characteristic_add(
          m_cus.service_handle, &char_md, &attr_char_value, &m_cus.custom_value_handles);
      APP_ERROR_CHECK(err_code);
    }
  }  // namespace

  void init() {
    m_cus.conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
    auto          err_code  = sd_ble_uuid_vs_add(&base_uuid, &m_cus.uuid_type);
    APP_ERROR_CHECK(err_code);

    const ble_uuid_t ble_uuid{
        .uuid = CUSTOM_SERVICE_UUID,
        .type = m_cus.uuid_type,
    };
    err_code =
        sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_cus.service_handle);
    APP_ERROR_CHECK(err_code);

    custom_value_char_add();
  }

  void ble_cus_custom_value_update(uint8_t custom_value) {
    NRF_LOG_INFO("updating timetable service value");

    ble_gatts_value_t gatts_value{
        .len     = sizeof(uint8_t),
        .offset  = 0,
        .p_value = &custom_value,
    };
    const auto err_code = sd_ble_gatts_value_set(
        m_cus.conn_handle, m_cus.custom_value_handles.value_handle, &gatts_value);
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::timetable_service