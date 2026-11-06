# Roadmap

Planned work for the bike LED wheel, roughly in priority order. Design reasoning lives in [docs/bike-hub-pov-lights.md](docs/bike-hub-pov-lights.md).

## Now: get v2 running on the bike

- [ ] Build `firmware/bike_wheel_lights_v2` in the Arduino IDE / `arduino-cli` (ESP32 core 3.3+, FastLED 3.10+) for both boards
- [ ] Bench test: IMU detected, Hall pulses counted, BLE name shows as `Itsy-Wheel-A`/`-B`
- [ ] Calibrate `HALL_TOP_LED` and `LED_DIRECTION` with debug modes 1 and 3 (`config.h`)
- [ ] Check `GYRO_AXLE_AXIS` / `GYRO_SIGN` match how the board is mounted
- [ ] Ride test: image stays upright at different speeds, while braking, and after stopping
- [ ] Tune `HALL_BLEND`, `SCALE_LEARN_RATE` and `LED_LATENCY_US` from what the ride test shows
- [ ] Set wheel ids with `W<n>X` and confirm ESP-NOW relays from A to B

## Next: make the iOS app match the firmware

- [ ] **Send separate settings to each wheel.** `WheelData.makeSendString()` only sends wheel one, so the Dual mode toggle and the wheel two pickers do nothing. Send `M..XM..X` when dual mode is on.
- [ ] Share one `BLEManager` between `ContentView` and `WheelEditor` so both screens show the same connection status
- [ ] Add the `D<n>X` debug views and `W<n>X` wheel id commands to the app, so calibration doesn't need nRF Connect
- [ ] Reconnect automatically to the last wheel used
- [ ] Clean up leftover template names (`ble_testApp`, `supamoto.ble-test`, "Picker" labels)

## Firmware improvements

- [ ] Remember the last mode/submode in flash, so a wheel comes back on showing the same effect
- [ ] Use ESP-NOW to sync animation timing between wheels, so effects line up front to rear
- [ ] Gravity fallback when the Hall sensor misses pulses: `atan2` of the high-passed in-plane accelerometer axes (the accelerometer is already on at 1.66 kHz for this)
- [ ] Hall-only fallback if the IMU fails, guessing speed changes from the last two periods
- [ ] Gamma-correct all palettes the same way. Only some are converted now, so brightness differs between palettes.
- [ ] Send status back to the phone over BLE notify (speed, Hall OK, battery)

## Two-wheel modes

Every mode today is for a single wheel; dual mode just runs two of them. Two-wheel modes are designed for the pair, so the front and rear wheels play one effect between them.

**First one: passing a lit rectangle.** It should look like the bike rides past a lit rectangle that stays still on the road. The front wheel lights up front to back, then turns off front to back. The rectangle can be up to one wheel wide. The rear wheel plays the same effect when it reaches the same spot, so its delay is wheelbase ÷ speed.

- [ ] Track distance on each wheel (gyro angle × tyre radius), not just time. If the rear wheel starts after it has rolled one wheelbase, the effect stays lined up when the bike speeds up or slows down. It also needs no shared clock.
- [ ] Each wheel needs to know whether it's front or rear: use the wheel id (`W<n>X`), with A = front
- [ ] Set wheelbase and tyre circumference in `config.h` (or from the app)
- [ ] Front wheel sends an ESP-NOW event at its own distance mark ("rectangle starts at distance D"); the rear wheel triggers when its own odometer reaches D + wheelbase
- [ ] Draw in the ground frame: lit region = the part of the ring between the rectangle's front and back edges, given where the wheel is relative to the rectangle
- [ ] Choose: rectangle width (up to one wheel), colour/palette, gap between rectangles
- [ ] New mode id in the firmware and the app (e.g. mode `5` = two-wheel, submodes = effects), sent as one command to both wheels
- [ ] More effects built on the same distance sync, e.g. a light that moves from the front wheel to the rear, or stripes painted on the road

## Hardware

- [x] Wiring diagram for both boards (Nano ESP32 and XIAO ESP32S3): https://claude.ai/artifact/ANaFYjWGwAm1XfpQkdb5yJ
- [ ] Battery voltage sense and a low-battery warning
- [ ] Neater mounting: printed enclosure, battery opposite the board for balance, printed magnet clip
- [ ] Sharper image at speed: 2–4 parallel data lines, or APA102/SK9822/HD107S LEDs (SPI + DMA, ~20 kHz PWM, no dashed dim colours)

## Later / ideas

- [ ] Show actual images (logos, text) instead of only palette sweeps: polar image format plus upload over BLE
- [ ] Web Bluetooth control page for Android (and Bluefy on iOS)
- [ ] Effects that react to speed (colour or animation rate follows wheel speed, brake flash when slowing down)
- [ ] Over-the-air firmware updates
