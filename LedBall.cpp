#include "LedBall.h"

int addTwoLedValues(int led1, int led2) {
    int lower = min(led1, led2);
    int higher = max(led1, led2);
    int result = higher;
    int diff = higher - lower;

    if (diff <= 9 && diff > 3) {
      result++;
    }
    if (diff <= 3 && diff > 1) {
      result = result + 2;
    }
    if (diff <= 1) {
      result = result + 3;
    }
   return min(result, 255);
}

LedBall::LedBall() {
      red = 0;
      green = 0;
      blue = 0;
    }

int LedBall::getRed() {
  return red;
}

int LedBall::getGreen() {
  return green;
}

int LedBall::getBlue() {
  return blue;
}

void LedBall::addLed(int red1, int green1, int blue1) {
  red = addTwoLedValues(red, red1);
  green = addTwoLedValues(green, green1);
  blue = addTwoLedValues(blue, blue1);
}

void LedBall::resetLedBall() {
  red = 0;
  green = 0;
  blue = 0;
}
