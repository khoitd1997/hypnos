#include <stdbool.h>
#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifndef _TIMETABLE_SERVICE_MODULE
#define _TIMETABLE_SERVICE_MODULE

#define BLE_CUS_DEF(_name) \
  static ble_cus_t _name;  \
  NRF_SDH_BLE_OBSERVER(_name##_obs, TIMETABLE_SERVICE_OBSERVER_PRIO, ble_cus_on_ble_evt, &_name)

// f364adc9-b000-4042-ba50-05ca45bf8abc
#define CUSTOM_SERVICE_UUID_BASE \
  { 0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0xC9, 0xAD, 0x64, 0xF3 }

#define CUSTOM_SERVICE_UUID 0x1400
#define CUSTOM_VALUE_CHAR_UUID 0x1401

enum ble_cus_evt_type_t { BLE_CUS_EVT_DISCONNECTED, BLE_CUS_EVT_CONNECTED };

struct ble_cus_evt_t {
  ble_cus_evt_type_t evt_type;
};

struct ble_cus_t;
typedef void (*ble_cus_evt_handler_t)(ble_cus_t* p_bas, ble_cus_evt_t* p_evt);

struct ble_cus_t {
  ble_cus_evt_handler_t    evt_handler;
  uint16_t                 service_handle;
  ble_gatts_char_handles_t custom_value_handles;
  uint16_t                 conn_handle;
  uint8_t                  uuid_type;
};

struct ble_cus_init_t {
  ble_cus_evt_handler_t evt_handler;
  uint8_t               initial_custom_value;
  ble_srv_cccd_security_mode_t
      custom_value_char_attr_md; /**< Initial security level for Custom characteristics attribute */
};

uint32_t ble_cus_init(ble_cus_t* p_cus, const ble_cus_init_t* p_cus_init);

void ble_cus_on_ble_evt(ble_evt_t const* p_ble_evt, void* p_context);

uint32_t ble_cus_custom_value_update(ble_cus_t* p_cus, uint8_t custom_value);

#endif