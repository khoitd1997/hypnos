# hypnos

Module embedded inside users' laptop to "remind" them to take break and sleep

## Background

I used to have a script that runs every couple hours to turn the computer off to remind me to take a break. However, that script can be easily bypassed and it's not cross-platform. Embedding ```hypnos``` into the motherboard of my laptop and enabling tamperproof protection in the BIOS for the case make it much harder to bypass.

## Features

- Interact with device through phone app
    - Allow users' actions to be asynchronous, save and transmit their actions when the device is connected
    - Allow a limited number of exceptions through tokens per month: Allow users to select a time range that they don't want to be disturbed, the number of tokens needed are automatically calculated and applied
    - Customizable work and break duration(there has to be an upper and lower limit though)
    - Statistics for tracking progress
    - Foreground task to show device status as well as current phase
    - Synchronize with app to keep correct time, remind users to connect to device if they haven't for a long time
    - Indicate the current time stretch
    - Bed time and wake time
    - Battery indicator
- Flexibly adapt to user' start of day
- Hard to abuse or bypass but also possible to remove when truly necessary
- Low power consumption
    - Can we use advertisement to send data?
    - Connect only when:
      - User has pending actions like changing rest interval
      - Need corrective actions such as clock syncing
- Reset switch for resetting all settings(including BLE bonding information)
- Small size: About 1.5 x 1.5 inch

## References

[BLE Chips Power Consumption](https://www.argenox.com/library/bluetooth-low-energy/bluetooth-le-chipset-guide-2019/#ble-device-comparison)
[Software RTC](https://github.com/NordicPlayground/nrf5-calendar-example)
[Directed Advertising](https://devzone.nordicsemi.com/f/nordic-q-a/39950/directed-advertising---how-to-make-it-work)
[Power Optimization](https://www.argenox.com/library/bluetooth-low-energy/ble-advertising-primer/)
[CR2032 and NRF52](https://devzone.nordicsemi.com/f/nordic-q-a/36982/cr2032-coin-cell-battery-life-estimation-with-nrf52-as-beacon)
[Picking Advertising Interval](https://www.beaconzone.co.uk/ibeaconadvertisinginterval)

![](image/2020-04-10-20-41-07.png)

While scanning in the foreground will likely immediately discover a device advertising next to it, discovery in the background can take up to ~60 times longer.
