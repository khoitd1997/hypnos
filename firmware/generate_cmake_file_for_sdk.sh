#!/bin/bash
# used for getting from nrf52 sdk makefiles into cmake files

set -e
curr_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd ${curr_dir}/nrf_sdk
make -f sdk_wrapper.mk
