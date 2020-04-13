# Hypnos Firmware

NRF52840 power consumption data, measured using a trueRMS multimeter with supply voltage of 3V, assuming bonded device:

| Type               | Adv Interval(0.625 ms) | Conn(s) | mA |
|--------------------|------------------------|---------|----|
| Slow               |                        |         |    |
| Fast               | 100                    | 2       |    |
| Directed           |                        |         |    |
| Directed High Duty |                        |         |    |

## References

The special event like BSP_EVENT_SYSOFF is assigned using:

```c
bsp_event_to_button_action_assign(BTN_ID_SLEEP,
                                  BSP_BUTTON_ACTION_LONG_PUSH,
                                  BSP_EVENT_SYSOFF);
```

[BSP Indication](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Fgroup__bsp.html)