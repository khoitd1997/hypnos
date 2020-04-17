#ifndef _PM_MODULE_HPP
#define _PM_MODULE_HPP

#include <cstdint>

namespace pm {
  void init();

  void delete_disconnected_bonds();
  void delete_curr_conn_bond(const uint16_t target_conn_handle);
  void delete_all_except_curr_bond(const uint16_t target_conn_handle);
  void delete_all_bonds();
}  // namespace pm

#endif  // ! _PM_MODULE_HPP