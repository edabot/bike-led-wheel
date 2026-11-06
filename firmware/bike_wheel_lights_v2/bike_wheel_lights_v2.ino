// Bike hub POV lights v2 — single ESP32-S3 per wheel.
//
//   Core 0: BLE from phone + ESP-NOW relay to other wheels      (Comms.cpp)
//   Core 1: tracker task, 1 kHz gyro + Hall angle tracking       (AngleTracker.cpp)
//           loop(): commands, animation steps, LED drawing        (this file + effects.ino)
//
// See config.h for pins and tuning, bike-hub-pov-lights.md for the design.

#include <FastLED.h>
#include <Preferences.h>
#include "config.h"
#include "AngleTracker.h"
#include "Comms.h"
#include "Spark.h"
#include "palettes.h"

#define NUM_SPARKS 200   // front fire keeps ~150 alive at once

// ---- LED buffers ----
CRGB leds[NUM_LEDS];       // what goes to the strip
CRGB image[NUM_LEDS];      // current scene; for effects, index 0 = top of wheel

// ---- display state ----
enum Scene { SCENE_IDLE, SCENE_EFFECT };
Scene shownScene = SCENE_IDLE;
bool fading = false;
uint32_t fadeStartMs = 0;

bool moving = false;
uint32_t selectionUntilMs = 0;   // preview selected effect while stopped

int displayMode = 0;
int paletteId = 0;
int sparkId = 0;
int fireballId = 0;
int debugMode = 0;     // 1: physical bar LEDs 1-5, 2: no rotation, 3: stabilised bar at top
bool isReversed = false;
uint8_t wheelId = DEFAULT_WHEEL_ID;

uint32_t nextStepUs = 0;
Preferences prefs;

// ---- palettes (palettes.h) ----
CRGBPalette256 currentPalette(gGradientPalettes[0]);
CRGBPalette256 fireballPalette(gFireballPalettes[0]);
CRGBPalette256 sparkPalette(gSparkPalettes[0]);

// ---- effects (effects.ino) ----
void initEffects();
void loadPaletteImage();
void stepEffect();
void stepIdleNoise();
void resetSparks();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nBike wheel lights v2");

  prefs.begin("wheel", false);
  wheelId = prefs.getUChar("id", DEFAULT_WHEEL_ID);
  Serial.printf("Wheel %c\n", 'A' + wheelId);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);   // dithering flickers frame-to-frame, bad for POV
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.clear(true);

  initEffects();

  if (!Tracker::begin()) {
    // No IMU: blink red so it's obvious on the wheel.
    for (;;) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      leds[0] = (millis() / 500) % 2 ? CRGB::Red : CRGB::Black;
      FastLED.show();
      delay(50);
    }
  }

  Comms::begin(wheelId);
  nextStepUs = micros();
}

// ---------------------------------------------------------------- commands --

void updateWheel(int mode, int subMode, bool reversed) {
  Serial.printf("Mode %d sub %d %s\n", mode, subMode, reversed ? "reversed" : "");
  displayMode = mode;
  isReversed = reversed;

  if (mode == 0) {
    paletteId = subMode % gGradientPaletteCount;
    currentPalette = gGradientPalettes[paletteId];
    loadPaletteImage();
  }
  if (mode == 1 || mode == 3 || mode == 4) {
    fireballId = subMode % gFireballPaletteCount;
    fireballPalette = gFireballPalettes[fireballId];
  }
  if (mode == 2) {
    sparkId = subMode % gSparkPaletteCount;
    sparkPalette = gSparkPalettes[sparkId];
  }
  selectionUntilMs = millis() + 1000;
}

void handleCommand(const Command& c) {
  Serial.printf("Cmd: %s\n", c.text);
  if (c.text[0] == 'D' && c.len >= 2) {
    debugMode = c.text[1] - '0';
    return;
  }
  if (c.text[0] == 'W' && c.len >= 2) {
    wheelId = (c.text[1] - '0') & 0x07;
    prefs.putUChar("id", wheelId);
    Serial.printf("Wheel id set to %c — restarting\n", 'A' + wheelId);
    delay(100);
    ESP.restart();
  }
  if (c.len < 4) return;   // prefix, mode, sub, 'X'
  int mode = c.text[1] - '0';
  int sub = c.text[2] - '0';
  if (mode < 0 || mode > 4 || sub < 0) return;
  updateWheel(mode, sub % 30, sub >= 30);
}

// USB serial: type e.g. M12X (or M12XM05X for two wheels) for bench testing.
void pollSerial() {
  static char buf[24];
  static size_t n = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (n) Comms::handlePhoneMessage(buf, n);
      n = 0;
    } else if (n < sizeof(buf)) {
      buf[n++] = ch;
    }
  }
}

// ---------------------------------------------------------------- render ----

void stepScene() {
  if (shownScene == SCENE_EFFECT) stepEffect();
  else stepIdleNoise();
}

// Cross-fade: fade current scene down to 20% over FADE_MS, switch, fade back up.
uint8_t sceneLevel(Scene wanted) {
  uint32_t nowMs = millis();
  if (!fading && wanted != shownScene) {
    fading = true;
    fadeStartMs = nowMs;
  }
  if (!fading) return 255;
  uint32_t t = nowMs - fadeStartMs;
  float level;
  if (t < FADE_MS) {
    level = 1.0f - (float)t / FADE_MS;
  } else {
    if (shownScene != wanted) shownScene = wanted;
    level = (float)(t - FADE_MS) / FADE_MS;
    if (t >= 2 * FADE_MS) fading = false;
  }
  level = constrain(level + 0.2f, 0.0f, 1.0f);
  return (uint8_t)(level * 255);
}

void loop() {
  Command cmd;
  while (Comms::receive(cmd)) handleCommand(cmd);
  pollSerial();

  uint32_t nowUs = micros();
  WheelState ws = Tracker::snapshot();
  float dps = fabsf(ws.omega) * RAD_TO_DEG;
  if (!moving && dps > MOVING_ON_DPS) moving = true;
  if (moving && dps < MOVING_OFF_DPS) moving = false;

  Scene wanted = (moving || (int32_t)(selectionUntilMs - millis()) > 0) ? SCENE_EFFECT : SCENE_IDLE;
  uint8_t level = sceneLevel(wanted);

  // Fixed-rate animation steps, independent of frame rate.
  int steps = 0;
  while ((int32_t)(nowUs - nextStepUs) >= 0) {
    if (++steps > 4) { nextStepUs = nowUs + ANIM_STEP_US; break; }
    stepScene();
    nextStepUs += ANIM_STEP_US;
  }

  // Where is the top of the wheel when these LEDs actually light up?
  float a = Tracker::predictAngle(ws, nowUs + LED_LATENCY_US);
  float topLed = HALL_TOP_LED - LED_DIRECTION * a * NUM_LEDS / TWO_PI;
  int offset = ((int)lroundf(topLed) % NUM_LEDS + NUM_LEDS) % NUM_LEDS;
  if (debugMode == 2) offset = 0;

  if (shownScene == SCENE_EFFECT) {
    for (int i = 0; i < NUM_LEDS; i++) leds[(offset + i) % NUM_LEDS] = image[i];
  } else {
    memcpy(leds, image, sizeof(leds));   // idle noise is not stabilised
  }
  if (level < 255) nscale8_video(leds, NUM_LEDS, level);

  if (debugMode == 1) fill_solid(leds + 1, 5, CRGB::Magenta);
  if (debugMode == 3) {
    for (int j = -2; j <= 2; j++) leds[(offset + j + NUM_LEDS) % NUM_LEDS] = CRGB::Magenta;
  }

  FastLED.show();

#if STATUS_PRINT_MS
  static uint32_t frames = 0, lastPrint = 0;
  frames++;
  if (millis() - lastPrint >= STATUS_PRINT_MS) {
    float secs = (millis() - lastPrint) / 1000.0f;
    Serial.printf("fps %.0f | gyro %.2f rev/s | hall %.2f rev/s #%lu | scale %.4f | %s\n",
                  frames / secs, ws.omega / TWO_PI, ws.hallRps, (unsigned long)ws.hallCount,
                  ws.gyroScale, moving ? "moving" : "stopped");
    frames = 0;
    lastPrint = millis();
  }
#endif
}
