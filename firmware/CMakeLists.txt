cmake_minimum_required(VERSION 3.14)
project(hypnos C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(platform_indepdendent_c_and_cpp_flags "-Og -Wall -DDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${platform_indepdendent_c_and_cpp_flags}")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${platform_indepdendent_c_and_cpp_flags} -std=c++17")

set(platform_indepdendent_srcs src/ble/service/time_exception_list.cpp)

include_directories(test)

if(NOT CMAKE_CROSSCOMPILING)
  set(test_exe_name doctest_exe)

  add_executable(${test_exe_name} ${platform_indepdendent_srcs}
                                  test/doctest_main.cpp)
  target_include_directories(${test_exe_name} PUBLIC src/ble)

  add_custom_target(
    run_doctest ALL
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_exe_name}
    DEPENDS ${test_exe_name})

else()
  find_program(nrfjprog_exe NAMES nrfjprog)
  find_program(nrfutil_exe NAMES nrfutil) # used in flash_program.sh
  find_program(java_exe NAMES java)

  set(common_flags
      "-D__HEAP_SIZE=8192 -D__STACK_SIZE=8192 -DDOCTEST_CONFIG_DISABLE")
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${common_flags}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${common_flags} -Wno-pointer-compare")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} ${common_flags} -fno-rtti -Wno-comment -fno-exceptions"
  )

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
    COMMAND ${nrfjprog_exe} -f nrf52 --program
            ${PROJECT_SOURCE_DIR}/s140_nrf52_7.0.1_softdevice.hex --sectorerase
    COMMAND ${nrfjprog_exe} -f nrf52 --reset)

  set(system_view_incs src/system_view/Config/ src/system_view/SEGGER/)
  set(system_view_srcs src/system_view/Config/SEGGER_SYSVIEW_Config_NoOS.c src/system_view/SEGGER/SEGGER_SYSVIEW.c)

  include_directories(SYSTEM ${NRF_INCS} ${system_view_incs})
  include_directories(src src/sdk_config src/ble src/ble/service/)

  function(firmware_executable_template board_name include_in_all)
    set(firmware_executable_base_name ${PROJECT_NAME}_${board_name})
    set(firmware_executable ${firmware_executable_base_name}.elf)
    set(src_files
        ${NRF_SRCS}
        ${system_view_srcs}
        ${platform_indepdendent_srcs}
        src/firmware_main.cpp
        src/power_module.cpp
        src/misc_module.cpp
        src/adc_module.cpp
        src/ble/advertising_module.cpp
        src/ble/qwr_module.cpp
        src/ble/pm_module.cpp
        src/ble/connection_module.cpp
        src/ble/gap_module.cpp
        src/ble/gatt_module.cpp
        src/ble/ble_module.cpp
        src/ble/service/bms_module.cpp
        src/ble/service/bas_module.cpp
        src/ble/service/timetable_service_module.cpp)
    add_executable(${firmware_executable} ${src_files})

    string(TOUPPER ${board_name} board_name_uppercase)
    target_compile_options(${firmware_executable}
                           PUBLIC -DBOARD_${board_name_uppercase})
    target_include_directories(${firmware_executable}
                               PUBLIC src/sdk_config/${board_name})
    target_link_options(${firmware_executable} PUBLIC
                        -T${CMAKE_SOURCE_DIR}/gcc_nrf52840.ld)

    if(include_in_all)
      add_custom_target(
        flash_${board_name} ALL
        COMMAND ${PROJECT_SOURCE_DIR}/flash_program.sh ${board_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${firmware_executable})

    else()
      add_custom_target(
        flash_${board_name}
        COMMAND ${PROJECT_SOURCE_DIR}/flash_program.sh ${board_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${firmware_executable})
    endif()
  endfunction()

  firmware_executable_template(pca10056 TRUE)
  firmware_executable_template(pca10059 FALSE)

endif()
