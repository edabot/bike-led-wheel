#ifndef LEDBALL_H
#define LEDBALL_H
#include <Arduino.h>

class LedBall {
  
  private:
    int red;
    int green;
    int blue;
    
  public:
    LedBall();
    int getRed();
    int getGreen();
    int getBlue();
    void addLed(int red1, int green1, int blue1);
    void resetLedBall();
};
#endif
