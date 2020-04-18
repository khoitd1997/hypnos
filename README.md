# hypnos

Module embedded inside users' laptop to "remind" them to take break and sleep

## Background

I used to have a script that runs every couple hours to turn the computer off to remind me to take a break. However, that script can be easily bypassed and it's not cross-platform. Embedding ```hypnos``` into the motherboard of my laptop and enabling tamperproof protection in the BIOS for the case make it much harder to bypass.

## Features

- Interact with device through phone app
    - Allow users' actions to be asynchronous, save and transmit their actions when the device is connected
    - Allow a limited number of exceptions through tokens per month: Allow users to select a time range that they don't want to be disturbed, the number of tokens needed are automatically calculated and applied
        - Waving sleep time as well as applying many tokens in the same day should cost more to prevent binge
    - Customizable work and break duration(there has to be an upper and lower limit though)
    - Limit amount of total screen time per day
    - Statistics for tracking progress
    - Foreground task to show device status as well as current phase
    - Synchronize with app to keep correct time, remind users to connect to device if they haven't for a long time
    - Indicate the current time stretch
    - Bed time and wake time
    - Battery indicator
- Flexibly adapt to user' start of day
- Hard to abuse or bypass but also possible to remove when truly necessary
- Low power consumption
- User has pending actions like changing rest interval
- Need corrective actions such as clock syncing
- Reset switch for resetting all settings(including BLE bonding information)
- Small size: About 1.5 x 1.5 inch

## Custom GATT

To store an hour:minute info, needs 10 bits(4 for hour, 6 for minute). All timing is stored that way except for the current time service

All would require authentication to read/write

| Handle | Type                  | Data Type      | Range                           |
|--------|-----------------------|----------------|---------------------------------|
|        | DAY_START             | time type      | 24-hour format                  |
|        | DAY_END               | time type      | 24-hour format                  |
|        | WORK_DURATION_MINUTE  | uint8          | 0-120                           |
|        | BREAK_DURATION_MINUTE | uint8          | 0-120                           |
|        | TODAY_EXCEPTION       | time type list | 0-5 items                       |
|        | TOKENS_LEFT           | uint8          | depend on policy implementation |

| Type                  | UUID                                 |
|-----------------------|--------------------------------------|
| DAY_START             | e3856dcf-d7df-43db-a796-c992f1751295 |
| DAY_END               | 0db59a75-ee5b-4fcf-abee-ab8b8e784719 |
| WORK_DURATION_MINUTE  | f1046dae-36ce-4859-b3db-ca9afeea973a |
| BREAK_DURATION_MINUTE | b069a498-4e92-476d-80ff-dce449536c7d |
| TODAY_EXCEPTION       | e78a37cc-5c28-4f1c-83b5-4160e38a7f31 |
| TOKENS_LEFT           | 206aa605-6ef9-4bb1-b825-8c180018c349 |

## References

[BLE Chips Power Consumption](https://www.argenox.com/library/bluetooth-low-energy/bluetooth-le-chipset-guide-2019/#ble-device-comparison)
[Software RTC](https://github.com/NordicPlayground/nrf5-calendar-example)
[Directed Advertising](https://devzone.nordicsemi.com/f/nordic-q-a/39950/directed-advertising---how-to-make-it-work)
[Power Optimization](https://www.argenox.com/library/bluetooth-low-energy/ble-advertising-primer/)
[CR2032 and NRF52](https://devzone.nordicsemi.com/f/nordic-q-a/36982/cr2032-coin-cell-battery-life-estimation-with-nrf52-as-beacon)
[Picking Advertising Interval](https://www.beaconzone.co.uk/ibeaconadvertisinginterval)

[Sending array in BLE data](https://devzone.nordicsemi.com/f/nordic-q-a/18040/ways-of-sending-a-float-array-as-a-value-of-a-characteristic)
[Current Time Service](https://stackoverflow.com/questions/35695711/how-to-correctly-set-the-date-and-time-in-a-bluetooth-low-energy-peripheral)

![](image/2020-04-10-20-41-07.png)
