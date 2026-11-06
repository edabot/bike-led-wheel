#pragma once
// ============================================================================
//  Bike hub POV lights v2 — configuration
//  Single ESP32-S3 board (Arduino Nano ESP32 or Seeed XIAO ESP32S3).
//
//  Core 0: BLE (phone) + ESP-NOW (other wheels)
//  Core 1: gyro + Hall angle tracker (1 kHz task) and LED rendering (loop)
// ============================================================================

// ---------------------------------------------------------------- Pins ------
// All pin numbers are raw GPIO numbers.
#if defined(ARDUINO_NANO_ESP32)
  // Arduino Nano ESP32: Tools > Pin Numbering > "By GPIO number (legacy)"
  #if defined(BOARD_HAS_PIN_REMAP) && !defined(BOARD_USES_HW_GPIO_NUMBERS)
    #error "Nano ESP32: set Tools > Pin Numbering > By GPIO number (legacy)"
  #endif
  #define LED_PIN    10   // D7
  #define HALL_PIN   5    // D2
  #define I2C_SDA    11   // A4
  #define I2C_SCL    12   // A5
#elif defined(ARDUINO_XIAO_ESP32S3)
  #define LED_PIN    1    // D0
  #define HALL_PIN   2    // D1
  #define I2C_SDA    5    // D4
  #define I2C_SCL    6    // D5
#else
  #error "Unknown board: add pin definitions in config.h"
#endif

// ---------------------------------------------------------------- Wheel -----
// 0 = wheel A (first 4 chars of an 8-char phone message), 1 = wheel B.
// Can be changed at runtime with the "W<n>X" command (saved to flash, reboot to rename BLE).
#define DEFAULT_WHEEL_ID   0

// ---------------------------------------------------------------- LEDs ------
#define NUM_LEDS           104
#define BRIGHTNESS         50      // 0-255 (same as v1)
#define MAX_MILLIAMPS      2500    // FastLED power limiter
#define LED_LATENCY_US     3500    // time from angle sample to LEDs latching (~30us/LED + reset)

// Which LED index is at the top of the wheel when the Hall sensor fires.
// Find it with debug mode 1 (lights LEDs 1..5) or adjust until debug mode 3's bar sits at the top.
#define HALL_TOP_LED       0
// +1 or -1: flip if the image spins at double speed instead of standing still.
#define LED_DIRECTION      1

// ---------------------------------------------------------------- IMU -------
// LSM6DS3 / LSM6DS3TR-C / LSM6DSO(X) / ISM330DHCX over I2C (0x6A or 0x6B, auto-detected)
#define GYRO_AXLE_AXIS     2       // 0=x 1=y 2=z : gyro axis parallel to the axle (v1 used z)
#define GYRO_SIGN          1       // +1 or -1 so forward riding gives positive omega (cosmetic)
#define GYRO_FS_DPS        2000    // 2000, or 4000 (ISM330DHCX only)
#define GYRO_DEADBAND_DPS  3.0f    // treat smaller rates as zero (bias residue)
#define TRACKER_PERIOD_MS  1       // tracker task rate (1 kHz)

// ---------------------------------------------------------------- Hall ------
#define USE_HALL           1       // 0 = gyro only (no drift correction)
#define HALL_ACTIVE_LOW    1       // DRV5023 pulls low when magnet present
#define HALL_MIN_PERIOD_US 50000   // ignore re-triggers faster than 20 rev/s
#define HALL_BLEND         0.5f    // fraction of angle error corrected per pulse (1.0 = snap)
#define SCALE_LEARN_RATE   0.1f    // how fast gyro scale is learned from Hall periods
#define HALL_TIMEOUT_US    3000000 // no pulse for this long -> next pulse snaps instead of blends

// ---------------------------------------------------------------- Motion ----
#define MOVING_ON_DPS      60.0f   // start showing effect above this wheel rate
#define MOVING_OFF_DPS     40.0f   // back to idle noise below this
#define FADE_MS            500     // each half of the idle <-> effect cross-fade

// Animations (sparks, fireballs, noise) step at a fixed rate so they look the same
// as v1 (~200 fps) no matter how fast frames are actually drawn.
#define ANIM_STEP_US       5000

// Front fire (mode 4): flames start at FIRE_FRONT_LED and spread both ways.
// Image index 0 = top of the wheel, so a quarter turn is the front or the back.
// If the fire shows at the back, use the Reverse toggle in the app (or change this).
#define FIRE_FRONT_LED       (NUM_LEDS / 4)
#define FIRE_SPAWN_PER_STEP  4     // new flame sparks per animation step (half each way)
#define FIRE_MAX_STRENGTH    25    // higher = flames reach further round the wheel

// ---------------------------------------------------------------- Radio -----
#define BLE_NAME_PREFIX    "Itsy-Wheel-"   // led-wheel-controller app lists names starting with "Itsy"
#define ESPNOW_CHANNEL     1

// ---------------------------------------------------------------- Debug -----
#define STATUS_PRINT_MS    1000    // 0 = off; prints fps / speed / Hall stats to USB serial
