#include "Spark.h"
#include <FastLED.h>

Spark::Spark(int value, float location) {
      this->location = location;
      this->value = value;
      sSpeed = -1 * float(random(10,30)) / 10;
    }

// Keep location in [0, numLeds). A tiny negative value such as -0.000001 plus
// 104 rounds to exactly 104.0 in float, which would give LED index 104 and write
// past the end of ledValues[]; the last check catches that.
void Spark::wrapLocation(int numLeds) {
  if (location >= numLeds) location -= numLeds;
  if (location < 0) location += numLeds;
  if (location >= numLeds || location < 0) location = 0;
}

void Spark::iterate(int numLeds) {
  sSpeed = constrain(sSpeed + .1, -4, 0);
  value = max(value - random(5,7), 0);
  location = location + sSpeed;
  wrapLocation(numLeds);
}
void Spark::iterateFireball(boolean isReversed, int numLeds) {
  if (!isReversed) {
    sSpeed = constrain(sSpeed - .05, 0, 4);    
  } else {
    sSpeed = constrain(sSpeed + .05, -4, 0);
  }
  value = max(value - 5, 0);
  location = location + sSpeed;
  wrapLocation(numLeds);
}

void Spark::iterateColorBall() {
  value = max(value - 1, 0);
}

int Spark::getLedIndex() {
  return floor(location);
}

int Spark::getValue() {
  return value;
}

float Spark::getLocation() {
  return location;
}

float Spark::getSpeed() {
  return sSpeed;
}

boolean Spark::isAlive() {
  return value > 0;
}

boolean Spark::isDead() {
  return value <= 0;
}

void Spark::setSpark(int value, float location, float sSpeed) {
  this->value = value;
  this->location = location;
  this->sSpeed = sSpeed;
}

void Spark::resetSpark(float location) {
  this->location = location;
  value = 100 + random(0,155);
  sSpeed = -1 * float(random(10,30)) / 10;
}

void Spark::killSpark() {
  value = 0;
}
