#!/bin/bash
set -e
curr_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd ${curr_dir}/build/$1

if [ "$1" = "pca10056" ]; then
    nrfjprog -f nrf52 --program hypnos_pca10056.hex --sectorerase --verify
    nrfjprog -f nrf52 --reset
fi

if [ "$1" = "pca10059" ]; then
    dev_file=""
    for sysdevpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
        syspath="${sysdevpath%/dev}"
        devname="$(udevadm info -q name -p $syspath)"
        [[ "$devname" == "bus/"* ]] && continue
        eval "$(udevadm info -q property --export -p $syspath)"
        [[ -z "$ID_SERIAL" ]] && continue

        if [[ "$devname" == *"ttyACM"* ]] && [[ "$ID_SERIAL" == *"Nordic_Semiconductor_Open_DFU_Bootloader"* ]]; then
            dev_file="/dev/$devname"
            break
        fi
    done

    [[ -z "$dev_file" ]] && echo "Can't find pca10059" && exit 1

    rm -f dfu.zip
    nrfutil pkg generate --hw-version 52 --sd-req 0x00CA --debug-mode --application hypnos_pca10059.hex dfu.zip

    # the first write usually fail for some reasons
    nrfutil dfu usb-serial -pkg dfu.zip -p $dev_file -b 115200
fi
