#pragma once
#include <Arduino.h>

// Phone -> BLE (Nordic UART service, same as the v1 Bluefruit boards) -> this unit,
// relayed to the other wheels over ESP-NOW. Runs on core 0 (BLE/WiFi stack tasks).
//
// Message format (unchanged from v1): one or more 'X'-terminated chunks, e.g.
//   "M12X"       -> one chunk, applied by every wheel
//   "M12XM05X"   -> chunk 0 for wheel A, chunk 1 for wheel B
// Chunk layout: [0]=prefix, [1]=mode digit, [2]=submode ('0'+n, +30 = reversed), 'X'.
// Special prefixes:  "D<n>X" debug mode n (all wheels),  "W<n>X" set this unit's wheel id (local only).

struct Command {
  char text[8];   // chunk including the trailing 'X', NUL-terminated
  uint8_t len;
};

namespace Comms {
  void begin(uint8_t wheelId);
  // Feed a message as if it came from the phone (used for USB serial testing).
  void handlePhoneMessage(const char* data, size_t len);
  // Non-blocking; called from the render loop on core 1.
  bool receive(Command& out);
}
