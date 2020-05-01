cmake_minimum_required(VERSION 3.14)
project(hypnos C CXX ASM)

find_program(nrfjprog_exe NAMES nrfjprog)
find_program(nrfutil_exe NAMES nrfutil) # used in flash_program.sh
find_program(java_exe NAMES java)

set(common_flags "-D__HEAP_SIZE=8192 -D__STACK_SIZE=8192 -DDEBUG")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${common_flags}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wno-pointer-compare ${common_flags} -Og -Wall")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${common_flags} -std=c++17 -fno-rtti -Wno-comment -Og -fno-exceptions"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(nrf_sdk)

add_custom_target(
  configure_cmsis
  COMMAND
    ${java_exe} -jar
    ${PROJECT_SOURCE_DIR}/nrf_sdk/sdk/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
    ${PROJECT_SOURCE_DIR}/src/sdk_config.h)

add_custom_target(erase_all COMMAND ${nrfjprog_exe} -f nrf52 --eraseall)

add_custom_target(
  flash_softdevice
  COMMAND ${nrfjprog_exe} -f nrf52 --program s140_nrf52_7.0.1_softdevice.hex
          --sectorerase
  COMMAND ${nrfjprog_exe} -f nrf52 --reset)

include_directories(SYSTEM ${NRF_INCS})
include_directories(src src/sdk_config src/ble)

function(firmware_executable_template board_name)
  set(firmware_executable_base_name ${PROJECT_NAME}_${board_name})
  set(firmware_executable ${firmware_executable_base_name}.elf)
  set(src_files
      ${NRF_SRCS}
      src/firmware_main.cpp
      src/power_module.cpp
      src/misc_module.cpp
      src/adc_module.cpp
      src/ble/advertising_module.cpp
      src/ble/bms_module.cpp
      src/ble/bas_module.cpp
      src/ble/timetable_service_module.cpp
      src/ble/qwr_module.cpp
      src/ble/pm_module.cpp
      src/ble/connection_module.cpp
      src/ble/gap_module.cpp
      src/ble/gatt_module.cpp
      src/ble/ble_module.cpp)
  add_executable(${firmware_executable} ${src_files})

  string(TOUPPER ${board_name} board_name_uppercase)
  target_compile_options(${firmware_executable}
                         PUBLIC -DBOARD_${board_name_uppercase})
  target_include_directories(${firmware_executable}
                             PUBLIC src/sdk_config/${board_name})
  target_link_options(${firmware_executable} PUBLIC
                      -T${CMAKE_SOURCE_DIR}/gcc_nrf52840.ld)

  add_custom_target(
    flash_${board_name}
    COMMAND ${PROJECT_SOURCE_DIR}/flash_program.sh ${board_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${firmware_executable})

endfunction()

firmware_executable_template(pca10056)
firmware_executable_template(pca10059)
