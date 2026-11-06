// Effects ported from v1 functions.ino.
// Each step*() advances one animation tick (ANIM_STEP_US) and writes image[].
// Rotation to keep the image upright is applied afterwards in loop().

// ---- circle geometry for idle noise ----
const float circleRadius = float(NUM_LEDS) / PI;
float xOffsets[NUM_LEDS];
float yOffsets[NUM_LEDS];

// ---- noise state (values as v1) ----
const int speedHue = 3;
const int scaleHue = 10;
const int speedSat = 6;
const int scaleSat = 30;
const int speedBright = 7;
const int scaleBright = 10;
float noiseX = 0, noiseY = 0;
float zHue = 100, zSat = 50, zBright = 0;
float colorStart = 0;
const float colorRange = .7;
const float colorSpeed = 80;
const int colorMax = 65536;

// ---- spark state ----
int ledValues[NUM_LEDS];
Spark* sparkArray[NUM_SPARKS];
uint32_t sparkGap = 0;
int lastSparkOrigin = 0;

CRGB paletteImage[NUM_LEDS];

void initEffects() {
  for (int i = 0; i < NUM_LEDS; i++) {
    float a = TWO_PI * i / NUM_LEDS;
    xOffsets[i] = cosf(a) * circleRadius;
    yOffsets[i] = sinf(a) * circleRadius;
    ledValues[i] = 0;
  }
  for (int i = 0; i < NUM_SPARKS; i++) sparkArray[i] = new Spark(0, 0);
  loadPaletteImage();
}

void loadPaletteImage() {
  float ledStep = 255.0f / (NUM_LEDS - 1);
  for (int i = 0; i < NUM_LEDS; i++) {
    paletteImage[i] = ColorFromPalette(currentPalette, (uint8_t)(i * ledStep), 255);
  }
}

void resetLedValues() {
  for (int i = 0; i < NUM_LEDS; i++) ledValues[i] = 0;
}

void resetSparks() {
  for (int i = 0; i < NUM_SPARKS; i++) sparkArray[i]->killSpark();
}

int addTwoFlameValues(int flame1, int flame2) {
  int lower = min(flame1, flame2);
  int higher = max(flame1, flame2);
  int result = higher;
  int diff = higher - lower;
  if (diff <= 9 && diff > 3) result++;
  if (diff <= 3 && diff > 1) result += 2;
  if (diff <= 1) result += 3;
  return min(result, 255);
}

void cleanUpSparks() {
  int index = 0;
  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() > 0) {
      sparkArray[index]->setSpark(sparkArray[i]->getValue(), sparkArray[i]->getLocation(),
                                  sparkArray[i]->getSpeed());
      if (i != index) sparkArray[i]->setSpark(0, 0, 0);
      index++;
    }
  }
}

// ---------------------------------------------------------------- palette ---
void stepPalette() {
  for (int i = 0; i < NUM_LEDS; i++) {
    image[i] = paletteImage[isReversed ? NUM_LEDS - 1 - i : i];
  }
}

// ---------------------------------------------------------------- sparks ----
void stepSparks() {
  int firstIndex = 0;
  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() == 0) { firstIndex = i; break; }
  }
  resetLedValues();

  if (sparkGap == 0 || millis() > sparkGap) {
    int newLedAmount = random(40, 60);
    int newOrigin = (random(0, NUM_LEDS - 50) + 25 + lastSparkOrigin) % NUM_LEDS;
    lastSparkOrigin = newOrigin;
    for (int i = 0; i < newLedAmount; i++) {
      if (firstIndex + i == NUM_SPARKS - 1) break;
      sparkArray[firstIndex + i]->resetSpark(newOrigin);
    }
    sparkGap = millis() + random(50, 300);
  }

  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() > 0) {
      sparkArray[i]->iterate(NUM_LEDS);
      int idx = sparkArray[i]->getLedIndex();
      ledValues[idx] = addTwoFlameValues(ledValues[idx], sparkArray[i]->getValue());
    }
  }
  cleanUpSparks();

  for (int i = 0; i < NUM_LEDS; i++) {
    image[i] = ColorFromPalette(sparkPalette, 255 - ledValues[i], 255);
  }
}

// ---------------------------------------------------------------- fireball --
void stepFireball(int numFireballs) {
  resetLedValues();

  int sparksToAdd = 2;
  for (int i = 0; i < NUM_SPARKS; i++) {
    if (!sparkArray[i]->isAlive()) {
      int maxStrength = numFireballs == 1 ? 30 : 23;
      sparksToAdd--;
      int strength = random(0, maxStrength);
      float sSpeed = float(strength) / 10;
      if (isReversed) sSpeed = -sSpeed;
      sparkArray[i]->setSpark(135 + strength * 4, random(numFireballs) * (NUM_LEDS / numFireballs), sSpeed);
      if (sparksToAdd == 0) break;
    }
  }

  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() > 0) {
      sparkArray[i]->iterateFireball(isReversed, NUM_LEDS);
      int idx = sparkArray[i]->getLedIndex();
      for (int j = 0; j < 3; j++) {
        int k = (idx + j) % NUM_LEDS;
        ledValues[k] = addTwoFlameValues(ledValues[k], sparkArray[i]->getValue());
      }
    }
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    image[i] = ColorFromPalette(fireballPalette, 255 - ledValues[i], 255);
  }
}

void stepEffect() {
  switch (displayMode) {
    case 0: stepPalette(); break;
    case 1: stepFireball(1); break;
    case 2: stepSparks(); break;
    case 3: stepFireball(2); break;
  }
}

// ---------------------------------------------------------------- idle ------
// Slow-moving noise shown when the wheel is stopped (v1 fillnoise).
static inline uint16_t n16(float v) { return (uint16_t)(int32_t)v; }

void stepIdleNoise() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t h = inoise8(n16(noiseX + xOffsets[i] * scaleHue), n16(noiseY + yOffsets[i] * scaleHue), n16(zHue));
    uint8_t s = inoise8(n16(noiseX + xOffsets[i] * scaleSat), n16(noiseY + yOffsets[i] * scaleSat), n16(zSat));
    uint8_t v = inoise8(n16(noiseX + xOffsets[i] * scaleBright), n16(noiseY + yOffsets[i] * scaleBright), n16(zBright));

    uint16_t hue = (uint16_t)((int32_t)(h * colorRange * 256 + colorStart) % colorMax);
    uint8_t sat = constrain(s + 70, 0, 255);
    uint8_t val = constrain(v - 20, 0, 255);

    CRGB c;
    hsv2rgb_spectrum(CHSV(hue >> 8, sat, val), c);
    image[i] = applyGamma_video(c, 2.6f);
  }

  zHue = fmodf(zHue + speedHue, 65536.0f);
  zSat = fmodf(zSat + speedSat, 65536.0f);
  zBright = fmodf(zBright + speedBright, 65536.0f);
  colorStart += colorSpeed;
  if (colorStart > colorMax) colorStart -= colorMax;
}
