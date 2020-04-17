#include "pm_module.hpp"

#include "app_error.h"

#include "ble_conn_state.h"

#include "peer_manager.h"
#include "peer_manager_handler.h"

#include "nrf_log.h"

#include "advertising_module.hpp"

#include "connection_module.hpp"

namespace ble::pm {
  namespace {
    constexpr uint8_t SEC_PARAM_BOND            = 0;
    constexpr uint8_t SEC_PARAM_MITM            = 1;
    constexpr uint8_t SEC_PARAM_LESC            = 0;
    constexpr uint8_t SEC_PARAM_KEYPRESS        = 0;
    constexpr uint8_t SEC_PARAM_IO_CAPABILITIES = BLE_GAP_IO_CAPS_DISPLAY_ONLY;
    constexpr uint8_t SEC_PARAM_OOB             = 0;   //!< Out Of Band data not available
    constexpr uint8_t SEC_PARAM_MIN_KEY_SIZE    = 7;   //!< Minimum encryption key size
    constexpr uint8_t SEC_PARAM_MAX_KEY_SIZE    = 16;  //!< Maximum encryption key size

    ble_conn_state_user_flag_id_t m_bonds_to_delete_flag;

    /**@brief Function for deleting a single bond if it does not belong to a connected peer.
     *
     * This will mark the bond for deferred deletion if the peer is connected.
     */
    void delete_single_bond(uint16_t conn_handle, void *p_context) {
      UNUSED_PARAMETER(p_context);
      ret_code_t   err_code;
      pm_peer_id_t peer_id;

      if (ble_conn_state_status(conn_handle) == BLE_CONN_STATUS_CONNECTED) {
        ble_conn_state_user_flag_set(conn_handle, m_bonds_to_delete_flag, true);
      } else {
        NRF_LOG_DEBUG("Attempting to delete bond.");
        err_code = pm_peer_id_get(conn_handle, &peer_id);
        APP_ERROR_CHECK(err_code);
        if (peer_id != PM_PEER_ID_INVALID) {
          err_code = pm_peer_delete(peer_id);
          APP_ERROR_CHECK(err_code);
          ble_conn_state_user_flag_set(conn_handle, m_bonds_to_delete_flag, false);
        }
      }
    }

    void pm_evt_handler(pm_evt_t const *p_evt) {
      pm_handler_on_pm_evt(p_evt);
      pm_handler_disconnect_on_sec_failure(p_evt);
      pm_handler_flash_clean(p_evt);

      switch (p_evt->evt_id) {
        case PM_EVT_CONN_SEC_SUCCEEDED: {
          pm_conn_sec_status_t conn_sec_status;

          // Check if the link is authenticated (meaning at least MITM).
          auto err_code = pm_conn_sec_status_get(p_evt->conn_handle, &conn_sec_status);
          APP_ERROR_CHECK(err_code);

          if (conn_sec_status.mitm_protected) {
            NRF_LOG_INFO("Link secured. Role: %d. conn_handle: %d, Procedure: %d",
                         ble_conn_state_role(p_evt->conn_handle),
                         p_evt->conn_handle,
                         p_evt->params.conn_sec_succeeded.procedure);
          } else {
            // The peer did not use MITM, disconnect.
            NRF_LOG_INFO("Collector did not use MITM, disconnecting");
            delete_single_bond(p_evt->conn_handle, NULL);
            err_code = sd_ble_gap_disconnect(p_evt->conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
          }
        } break;

        case PM_EVT_CONN_SEC_FAILED:
          connection::set_handle(BLE_CONN_HANDLE_INVALID);
          break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
          advertising::start();
          break;

        case PM_EVT_BONDED_PEER_CONNECTED:
          NRF_LOG_INFO("bonded peer connected");
          break;

        case PM_EVT_CONN_SEC_CONFIG_REQ: {
          NRF_LOG_INFO("already bonded peer request bonding");
          pm_conn_sec_config_t conn_sec_config = {.allow_repairing = true};
          pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        default:
          break;
      }
    }
  }  // namespace

  void init() {
    auto err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    m_bonds_to_delete_flag = ble_conn_state_user_flag_acquire();

    ble_gap_sec_params_t sec_param{};
    sec_param.bond         = SEC_PARAM_BOND;
    sec_param.mitm         = SEC_PARAM_MITM;
    sec_param.lesc         = SEC_PARAM_LESC;
    sec_param.keypress     = SEC_PARAM_KEYPRESS;
    sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob          = SEC_PARAM_OOB;
    sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;

    //   sec_param.kdist_own.enc  = 1;
    //   sec_param.kdist_own.id   = 1;
    //   sec_param.kdist_peer.enc = 1;
    //   sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
  }

  void delete_disconnected_bonds() {
    ble_conn_state_for_each_set_user_flag(m_bonds_to_delete_flag, delete_single_bond, NULL);
  }

  void delete_curr_conn_bond(const uint16_t target_conn_handle) {
    NRF_LOG_INFO("Client requested that bond to current device deleted");
    ble_conn_state_user_flag_set(target_conn_handle, m_bonds_to_delete_flag, true);
  }

  void delete_all_except_curr_bond(const uint16_t target_conn_handle) {
    NRF_LOG_INFO("Client requested that all bonds except current bond be deleted");

    pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    while (peer_id != PM_PEER_ID_INVALID) {
      uint16_t   conn_handle;
      const auto err_code = pm_conn_handle_get(peer_id, &conn_handle);
      APP_ERROR_CHECK(err_code);

      /* Do nothing if this is our own bond. */
      if (conn_handle != target_conn_handle) { delete_single_bond(conn_handle, NULL); }

      peer_id = pm_next_peer_id_get(peer_id);
    }
  }

  void delete_all_bonds() {
    NRF_LOG_INFO("Client requested that all bonds be deleted");

    pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    while (peer_id != PM_PEER_ID_INVALID) {
      uint16_t   conn_handle;
      const auto err_code = pm_conn_handle_get(peer_id, &conn_handle);
      APP_ERROR_CHECK(err_code);

      delete_single_bond(conn_handle, NULL);

      peer_id = pm_next_peer_id_get(peer_id);
    }
  }

  void delete_all_bonds_unsafe() {
    NRF_LOG_INFO("Erasing all bonds!");

    const auto err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
  }
}  // namespace ble::pm