# Bike Hub POV Light – Design Notes

A ring of LEDs mounted on a bicycle hub that displays an image which appears to stay still while the wheel spins. Multiple units (e.g. front and rear wheels) are controlled from a phone over BLE.

---

## 1. The Problem

The original build used an Arduino Nano 33 IoT and its accelerometer to estimate wheel speed, then shifted the image around the ring to cancel the rotation. The image drifted because **angle was obtained by integrating speed** – every small error in speed, timing, or sample rate accumulates, so the image slowly walks around the wheel.

**Fix:** measure an **absolute "up" reference** once per revolution and use a fast speed sensor only to fill in between references.

---

## 2. What the Sensors Actually See

### Accelerometer (rotating with the wheel)
| Component | What it looks like | What it tells you |
|---|---|---|
| Centripetal (r·ω², toward axle) | Constant offset (DC) – same at every wheel position | Speed |
| Gravity (1 g) | Sinusoid at the rotation frequency on the two in-plane axes | Angle / which way is up |
| Bumps, braking, vibration | Noise, mostly tens of Hz | Nothing useful |

- Use the **two axes in the plane of the wheel** (perpendicular to the axle). The axle axis sees ~no gravity.
- Angle from gravity: high-pass each in-plane axis (subtract a one-revolution running mean), then `θ = atan2(ay_hp, ax_hp)`.
- Leaning in turns reduces the gravity amplitude by cos(lean) but doesn't change its phase.
- Note: the top of a rolling wheel moves faster *relative to the ground*, but at constant speed the acceleration a point feels is constant in magnitude all the way around. The varying signal is gravity, not speed variation.

### Gyroscope
- The axle-direction axis reads angular velocity (ω) directly – the cleanest, fastest speed signal.
- Integrates to a smooth angle over short periods but drifts long-term.

### Hall-effect sensor + magnet on the fork
- One exact, noise-free "up" pulse per revolution, immune to bumps and braking.
- On its own, the angle between pulses must be estimated from the last period, so speed changes mid-revolution aren't seen until the next pulse (visible snaps under hard braking).

### Chosen approach: **Gyro + Hall**
- Gyro moves the image between pulses (reacts instantly to speed changes).
- Hall pulse corrects any drift once per revolution and can slowly calibrate the gyro scale.

---

## 3. Wheel Speeds (29" wheel, circumference ≈ 2.31 m)

Rule of thumb: **rev/s ≈ mph × 0.193**

| Speed | Rev/s (Hz) | RPM | °/s |
|---|---|---|---|
| 0 mph | 0 | 0 | 0 |
| 2 mph | 0.39 | 23 | 139 |
| 5 mph | 0.97 | 58 | 348 |
| 10 mph | 1.93 | 116 | 695 |
| 15 mph | 2.90 | 174 | 1,043 |
| 20 mph | 3.86 | 232 | 1,391 |

- A ±2000 °/s gyro is good to ~29 mph; ±4000 °/s (e.g. ISM330DHCX) doubles that.
- Below ~2 mph the wheel turns slower than once per second and the POV effect breaks down.
- Measure real rollout (mark tire, roll one turn) for exact numbers.

Reference: a 25" diameter circle has a circumference of ≈ 78.5 in / 1.99 m.

---

## 4. Sensor Sample Rates

| Sensor | Max rate |
|---|---|
| LSM6DS3 accelerometer | up to 6.66 kHz |
| LSM6DS3 gyroscope | up to 1.66 kHz |
| ISM330DHCX (accel + gyro) | up to 6.66 kHz |
| Hall switch (DRV5023 etc.) | µs-level response – use an interrupt + `micros()` |
| Reed switch | a few hundred Hz, needs debouncing |

- The stock Arduino_LSM6DS3 library defaults to ~104 Hz – too coarse. Configure the IMU for ~833 Hz – 1.66 kHz and read via its FIFO.
- Filtering: a one-revolution average removes the DC (centripetal) part at any speed; a gentle ~10–15 Hz low-pass removes most vibration.

---

## 5. LEDs (100 × WS2812B on one data line)

**Update rate:** ~30 µs per LED + ~280 µs reset ≈ 3.3 ms per update → ~300 updates/s.

| Speed | Image step per update |
|---|---|
| 10 mph | ~0.6 LED |
| 16 mph | ~1 LED |
| 20 mph | ~1.3 LEDs |
| 30 mph | ~1.9 LEDs |

- Single-pixel accuracy isn't required, so one data line is fine. Each update is still placed at the correct angle; the image just gets slightly softer at speed.
- **Latency compensation:** WS2812Bs show new data only after the whole chain is loaded (~3.3 ms ≈ 4.6° at 20 mph). Always draw for `angle + ω × latency`.
- **Internal PWM:** older WS2812Bs blink at ~400 Hz, so dim colors can look dashed at speed. Full-brightness channels are unaffected.
- **Timing:** avoid LED drivers that disable interrupts (they delay Hall timestamps). Use FastLED on the ESP32's RMT hardware.

Future upgrades: split into 2–4 parallel data lines, or switch to APA102/SK9822/HD107S (SPI + DMA, thousands of updates/s, ~20 kHz PWM).

---

## 6. System Architecture

```
 Phone ──BLE──► Unit A ──ESP-NOW broadcast──► Unit B, C, ...
```

- **Phone → one unit:** BLE (any unit can be the one the phone connects to).
- **Unit → other units:** ESP-NOW – no pairing, low latency, one broadcast reaches all units, runs alongside BLE.
- ESP-NOW can also carry timing data so multiple wheels run animations in sync.
- Why not the Nano 33 IoT: its BLE runs on a separate module over a serial link, simultaneous phone-connection + relay is shaky, and it competes with LED updates for CPU time.

### Phone side
- Testing: **nRF Connect** (iOS/Android) to write BLE characteristics without building an app.
- Later: a custom app, or a Web Bluetooth page (Chrome on Android; Bluefy on iOS).

---

## 7. Hardware (per unit)

### Electronics
| Part | Notes |
|---|---|
| Arduino Nano ESP32 *or* Seeed XIAO ESP32S3 | Dual core: one core for BLE/ESP-NOW, one for angle tracking + LEDs |
| IMU breakout: LSM6DS3TR-C, LSM6DSOX, or ISM330DHCX | I2C; gyro set to ±2000 °/s (or ±4000 °/s on ISM330DHCX) |
| Hall sensor: DRV5023 (or low-power DRV5032) | 3.3 V compatible, open-drain output |
| 10 kΩ resistor | Hall output pull-up to 3.3 V; output to an interrupt-capable pin |
| Neodymium disc magnet, ~6–10 mm | On the fork; unipolar sensors respond to one pole only – check which face triggers |
| 100 × WS2812B ring | One data line |
| 74AHCT125 level shifter | 3.3 V data → 5 V for reliable WS2812B input |
| 330–470 Ω resistor | Series on the data line near the first LED |
| 1000 µF capacitor | Across LED 5 V and GND near the ring |

### Power
- 5 V source that rotates with the wheel: small USB power bank or LiPo + 5 V boost converter.
- 100 WS2812Bs can draw up to ~6 A at full white – cap brightness in software (25–40%) so a 2–3 A supply is enough.
- Tie all grounds together (LEDs, board, sensors).

### Mechanical
- Hall sensor on the rotating hub assembly at a radius that passes close to the fork leg.
- Magnet on the fork at the same radius, gap of a few mm or less (epoxy, hot glue, or printed clip).
- Put the battery opposite the board to balance the wheel.
- Secure everything firmly – up to ~230 RPM plus road vibration.

---

## 8. Firmware Outline

**Core 0:** BLE (receive settings/animations from phone) + ESP-NOW (relay to other units).
**Core 1:** IMU reads, angle tracking, LED drawing.

### Angle tracking (gyro + Hall)

```cpp
volatile uint32_t hallTime = 0;
volatile bool hallFlag = false;

void IRAM_ATTR hallISR() { hallTime = micros(); hallFlag = true; }

float angle = 0;            // radians
float gyroScale = 1.0;      // learned correction
float angleAtLastHall = 0;

void loop() {
  // each IMU sample:
  float omega = readGyroAxle() * gyroScale;   // rad/s
  angle += omega * dt;                        // dt from micros()

  if (hallFlag) {
    hallFlag = false;
    float turned = angle - angleAtLastHall;   // gyro's estimate of one revolution
    if (turned > 0.5 * TWO_PI)                // sanity check
      gyroScale *= 1 + 0.1 * (TWO_PI / turned - 1);   // slowly learn scale
    float err = wrapPi(HALL_ANGLE - angle);
    angle += 0.3 * err;                       // blend (or snap: angle = HALL_ANGLE)
    angleAtLastHall = angle;
  }

  drawRing(wrapTo2Pi(angle + omega * LED_LATENCY));  // latency compensation
}
```

### Hall-only fallback
`angle = 2π × (time since last pulse) / last period` – drift-free, but snaps under hard braking. Can be improved by extrapolating acceleration from the last two periods.

### Gravity-based fallback (no Hall sensor)
Use `atan2` of the high-passed in-plane accelerometer axes as the absolute reference in place of the Hall pulse, with a small correction gain (e.g. 0.02) since it is noisy.

---

## 9. Priorities

1. **Hall sensor** – eliminates long-term drift.
2. **Gyro between pulses** – keeps the image steady through speed changes.
3. **Latency compensation** – stops the image creeping backward as speed rises.
4. Non-blocking LED output (FastLED/RMT on ESP32) – removes timing jitter.
5. Optional: parallel data lines or APA102-type LEDs for a sharper image.
