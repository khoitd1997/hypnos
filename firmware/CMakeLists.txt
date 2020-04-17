cmake_minimum_required(VERSION 3.14)
project(hypnos C CXX ASM)

# message("Flags are ${CMAKE_EXE_LINKER_FLAGS}")

set(common_flags "-D__HEAP_SIZE=8192 -D__STACK_SIZE=8192")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${common_flags}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wno-pointer-compare ${common_flags} -Og -Wall")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${common_flags} -std=c++17 -fno-rtti -Wno-comment -Og -fno-exceptions"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(nrf_sdk)

include_directories(SYSTEM ${NRF_INCS})

add_executable(
  ${PROJECT_NAME}_${BOARD_NAME}.elf
  ${NRF_SRCS}
  src/firmware_main.cpp
  src/advertising_module.cpp
  src/bms_module.cpp
  src/qwr_module.cpp
  src/pm_module.cpp
  src/power_module.cpp
  src/connection_module.cpp
  src/ble_module.cpp)
target_link_libraries(${PROJECT_NAME}_${BOARD_NAME}.elf)
target_include_directories(
  ${PROJECT_NAME}_${BOARD_NAME}.elf PUBLIC src/sdk_config
                                           src/sdk_config/${BOARD_NAME} src)
target_link_options(${PROJECT_NAME}_${BOARD_NAME}.elf PUBLIC
                    -T${CMAKE_SOURCE_DIR}/gcc_${BOARD_NAME}.ld)
