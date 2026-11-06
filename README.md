# bike-led-wheel

LED ring on a bicycle hub that shows an image that stays upright while the wheel spins (persistence of vision), controlled from an iPhone over BLE.

## Layout

| Path | What |
|---|---|
| `firmware/bike_wheel_lights_v2/` | Current firmware: one ESP32-S3 per wheel (Arduino Nano ESP32 or XIAO ESP32S3). Gyro + Hall angle tracking, BLE from the phone, ESP-NOW between wheels. |
| `firmware/v1/bike_wheel_lights_16/` | Original firmware for the Arduino Nano 33 IoT (gyro only, LEDs + sensor). |
| `firmware/v1/dual_bleuart_wheel/`, `firmware/v1/ble_to_led/` | Original nRF52 BLE boards that relayed phone commands to the Nano over serial. |
| `ios-app/` | SwiftUI controller app (`led-wheel-ble.xcodeproj`). |
| `docs/bike-hub-pov-lights.md` | Design notes: sensors, speeds, LEDs, architecture, hardware. |

## Building the firmware

Arduino IDE or `arduino-cli` with the Espressif ESP32 core (3.3+) and FastLED (3.10+).

- **Nano ESP32:** board `esp32:esp32:nano_nora`, and set Tools > Pin Numbering > *By GPIO number (legacy)*.
- **XIAO ESP32S3:** board `esp32:esp32:XIAO_ESP32S3`.

Pins and tuning are in `firmware/bike_wheel_lights_v2/config.h`.

## Phone protocol

The app writes ASCII to the Nordic UART service (`6E400001-…`, write characteristic `6E400002-…`). Wheels advertise as `Itsy-Wheel-A` / `Itsy-Wheel-B`.

Each command is 4 characters: `M`, mode digit, submode character, `X`.

- Mode: `0` pattern, `1` fireball, `2` sparks, `3` double fireball, `4` front fire
- Submode: `'0' + index`; add 30 for reversed
- One command (`M12X`) applies to every wheel. Two commands (`M12XM05X`) send the first to wheel A and the second to wheel B.
- Extra: `D<n>X` sets the debug view on all wheels, `W<n>X` sets the connected unit's wheel id (saved, then it reboots).

The unit the phone connects to applies its own command and relays the whole message to the other wheels over ESP-NOW.
