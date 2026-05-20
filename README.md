# XIAO MG24 BLE Temperature Monitor

A Bluetooth LE project for the [Seeed Studio XIAO MG24](https://wiki.seeedstudio.com/xiao_mg24_getting_started/) that broadcasts the chip's internal temperature every 3 seconds and displays it on a clean web page via Web Bluetooth — no app install required.

---

## Hardware

| Item | Detail |
|------|--------|
| Board | Seeed Studio XIAO MG24 |
| Built-in LED | Pin `PA7` |
| Antenna | Onboard (built-in) |

---

## How it works

The Arduino sketch runs a BLE GATT server on the XIAO MG24. Every 3 seconds it reads the chip's internal temperature with `getCPUTemp()` and sends it as a plain-text notification (e.g. `"27.50 C"`) over a custom notify characteristic. The onboard antenna is used by keeping `RF_SW_PIN` low:

```cpp
digitalWrite(RF_SW_PIN, LOW); // LOW = built-in antenna, HIGH = external antenna
```

The companion web page connects to the device via the Web Bluetooth API and displays the live temperature in the browser — no app install needed.

---

## BLE Profile

| Property | Value |
|----------|-------|
| Device name | `XIAO_MG24 Server` |
| Service UUID | `de8a5aac-a99b-c315-0c80-60d4cbb51224` |
| Notify characteristic UUID | `61a885a4-41c3-60d0-9a53-6d652a70d29c` |
| LED control characteristic UUID | `5b026510-4088-c297-46d8-be6c736a087a` |
| Notification format | Plain text, e.g. `27.50 C` |
| Notification interval | Every 3 seconds |

---

## Files

```
├── MG24-bluetooth-server.ino   # Arduino sketch (BLE server + temperature)
└── xiao-temperature.html       # Web Bluetooth client page
```

---

## Getting started

### Arduino sketch

1. Install the **XIAO MG24** board package in Arduino IDE
2. Under **Tools → Protocol stack**, select **BLE (Silabs)**
3. Open `MG24-bluetooth-server.ino` and upload to the board

### Web page

1. Open `xiao-temperature.html` in **Chrome on Android** (Web Bluetooth requires Chrome — Firefox and Samsung Internet are not supported)
2. Tap **Connect** and select `XIAO_MG24 Server`
3. The page will display the live temperature, updated every 2 seconds, with a colour indicator and a bar chart of the last 20 readings

---

## Requirements

- Arduino IDE with Silicon Labs XIAO MG24 board support
- Chrome on Android for the web client
- No extra libraries needed — uses the built-in Silicon Labs BLE stack

---

## References

- [XIAO MG24 Bluetooth usage — Seeed Wiki](https://wiki.seeedstudio.com/xiao_mg24_bluetooth/)
- [Silicon Labs BLE Stack API](https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/)
- [Web Bluetooth API — MDN](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API)
