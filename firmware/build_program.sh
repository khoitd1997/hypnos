#!/bin/bash
set -e
curr_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# generate cmake files from sdk
cd ${curr_dir}/nrf_sdk
make -f sdk_wrapper.mk

cd ${curr_dir}
mkdir -p build/$1 
cd build/$1 

# do build
cmake ${curr_dir} -DBOARD_NAME=$1 -DCMAKE_TOOLCHAIN_FILE=${curr_dir}/nrf_sdk/toolchain_$1.cmake && cmake --build .

# flash the firmware
arm-none-eabi-objcopy -O ihex hypnos_$1.elf hypnos_$1.hex