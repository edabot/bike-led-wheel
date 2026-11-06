#pragma once
#include <Arduino.h>

// Wheel angle from gyro (between pulses) + Hall sensor (absolute reference once per rev).
// Runs as a 1 kHz FreeRTOS task pinned to core 1.

struct WheelState {
  float    angle;       // radians [0, 2π); 0 = position where the Hall sensor fires
  float    omega;       // rad/s, signed, scale-corrected
  uint32_t timeUs;      // micros() when angle/omega were valid
  float    gyroScale;   // learned correction factor
  float    hallRps;     // rev/s measured from the last two Hall pulses
  uint32_t hallCount;
  bool     imuOk;
};

namespace Tracker {
  bool begin();                 // init IMU + Hall, start task. false if IMU not found.
  WheelState snapshot();        // thread-safe copy of the latest state
  // Angle predicted for time `tUs` (e.g. now + LED latency).
  float predictAngle(const WheelState& s, uint32_t tUs);
}
