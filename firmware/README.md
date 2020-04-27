# Hypnos Firmware

## Tools

Make sure ```nRF Command Line Tools```, ```JLink Driver``` and ```nrfutil```(pip) are installed

## Power Consumption Data

NRF52840 power consumption data, measured using a trueRMS multimeter with supply voltage of 3V, assuming bonded device. At the longest adv interval, the device is still detected and connected pretty fast, although to the time connect is inconsistent.

| Type | Adv Interval(0.625 ms) | uA  |
|------|------------------------|-----|
| Slow | 50                     | 250 |
| Slow | 1000                   | 70  |
| Slow | 3000                   | 50  |
| Slow | 10000                  | 40  |

## State Machine

[Link](https://www.lucidchart.com/documents/view/bfb6aadf-c374-40c3-be28-2cf55af65832/0_0)

## References

The special event like BSP_EVENT_SYSOFF is assigned using:

```c
bsp_event_to_button_action_assign(BTN_ID_SLEEP,
                                  BSP_BUTTON_ACTION_LONG_PUSH,
                                  BSP_EVENT_SYSOFF);
```

[BSP Indication](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Fgroup__bsp.html)
[BLE Security](https://duo.com/decipher/understanding-bluetooth-security)

[DCDC vs LDO power converter](https://devzone.nordicsemi.com/f/nordic-q-a/8106/internal-dcdc-vs-ldo-for-nrf52-series)

[Sending array in BLE data](https://devzone.nordicsemi.com/f/nordic-q-a/18040/ways-of-sending-a-float-array-as-a-value-of-a-characteristic)

[Software RTC](https://github.com/NordicPlayground/nrf5-calendar-example)
[Directed Advertising](https://devzone.nordicsemi.com/f/nordic-q-a/39950/directed-advertising---how-to-make-it-work)
[Power Optimization](https://www.argenox.com/library/bluetooth-low-energy/ble-advertising-primer/)
[CR2032 and NRF52](https://devzone.nordicsemi.com/f/nordic-q-a/36982/cr2032-coin-cell-battery-life-estimation-with-nrf52-as-beacon)