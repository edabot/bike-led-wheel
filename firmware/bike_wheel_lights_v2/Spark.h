#ifndef SPARK_H
#define SPARK_H
#include <Arduino.h>

class Spark {
  
  private:
    float location;
    float sSpeed;
    int value;
    
  public:
    Spark(int value, float location);
    void iterate(int numLeds);
    void iterateFireball(boolean isReversed, int numLeds);
    void iterateColorBall();
    int getLedIndex();
    int getValue();
    boolean isAlive();
    boolean isDead();
    float getLocation();
    float getSpeed();
    int getWidth();
    void setSpark(int value, float location, float sSpeed);
    void resetSpark(float location);
    void killSpark();
};
#endif
