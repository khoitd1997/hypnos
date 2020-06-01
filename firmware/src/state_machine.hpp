/**
 * @file state_machine.hpp
 * @author Khoi Trinh
 * @brief Include functions used for implementing the state machine
 *
 */

#ifndef _STATE_MACHINE_HPP
#define _STATE_MACHINE_HPP

namespace state_machine {
  bool is_in_break();

  void start_work_period();
  void end_work_period(const bool create_time_stamp);
}  // namespace state_machine

#endif