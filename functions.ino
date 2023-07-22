
void colorBand(int startLocation, float brightness) {
  int redBrightness = 255 * brightness;
  uint32_t red = strip.Color(redBrightness,   0,   0);
  for (int i = startLocation; i < startLocation + 30; i++) {
    strip.setPixelColor(i % NUM_LEDS, red);
  }
}

void palette(int startLocation) {
  for ( int i = 0; i < NUM_LEDS; i++) {
    int paletteIndex = i;
    if (isReversed) { paletteIndex = NUM_LEDS - 1 - paletteIndex; };
    int p = 3 * paletteIndex;
    strip.setPixelColor((startLocation + i) % NUM_LEDS, paletteRGBValues[p], paletteRGBValues[p+1], paletteRGBValues[p+2] );
  }
}

void loadPaletteToArray(float brightness) {
  int ledBrightness = 255 * brightness;
  float ledStep = 255.0 / (NUM_LEDS - 1);
  for ( int i = 0; i < NUM_LEDS; i++) {
    int location = (int) i * ledStep;
    CRGB color = ColorFromPalette( currentPalette, location, ledBrightness);
    paletteRGBValues[i*3] = color[0];
    paletteRGBValues[i*3 + 1] = color[1];
    paletteRGBValues[i*3 + 2] = color[2];
  }
}

void fillnoise(float brightness)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    int noiseValH = inoise8(x + xOffsets[i] * scaleHue, y + yOffsets[i] * scaleHue, zHue);
    int noiseValS = inoise8(x + xOffsets[i] * scaleSat, y + yOffsets[i] * scaleSat, zSat);
    int noiseValV = inoise8(x + xOffsets[i] * scaleBright, y + yOffsets[i] * scaleBright, zBright);

    int hue = (int) (noiseValH * colorRange * 256 + colorStart);

    if (mode == "TRIAD") {
      if (noiseValH > colorMax * 5 / 8) {
        hue = hue + colorMax * 2 / 3 - colorMax * colorRange * 5 / 8;
      } else if (noiseValH > colorMax * 3 / 8) {
        hue = hue + colorMax / 3 - colorMax * colorRange * 3 / 8;
      }
    }
    if (mode == "COMPLEMENTARY") {
      if (noiseValH > colorMax / 2) {
        hue = hue + colorMax / 2 - colorMax * colorRange / 2;
      }
    }
    hue = hue % colorMax;

    int saturation = constrain(noiseValS + 70, 0, 255);
    int value = constrain(noiseValV - 20, 0, 255) * brightness;

    uint32_t rgbcolor = strip.gamma32(strip.ColorHSV(hue, saturation, value));
    strip.setPixelColor(i, rgbcolor);
  }

  zHue += speedHue;
  zSat += speedSat;
  zBright += speedBright;

  // apply slow drift to X and Y, just for visual variation.
  x += speedSat / 8;
  y -= speedSat / 16;

  colorStart += colorSpeed;
  if (colorStart > colorMax) {
    colorStart -= colorMax;
  }

}

void colorBandFade(int startLocation, int counter) {
  if (counter < 500) {
    float brightness = (500 - (float)counter) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    colorBand(startLocation, brightness);
  }
  else {
    float brightness = ((float)counter - 500) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    colorBand(startLocation, brightness);
  }
}

void paletteFade(int startLocation, int counter) {
  if (counter < 500) {
    float brightness = (500 - (float)counter) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    palette(startLocation);
  }
  else {
    float brightness = ((float)counter - 500) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    palette(startLocation);
  }
}


void fillNoiseFade(int counter) {
  if (counter < 500) {
    float brightness = (500 - (float)counter) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    fillnoise(brightness);
  }
  else {
    float brightness = ((float)counter - 500) / 500;
    brightness = constrain(brightness + .2, 0, 1);
    fillnoise(brightness);
  }
}

void referenceBar(int length) {
  strip.fill(strip.Color(255, 0, 255), 1, length);
}

void sparks(int startLocation) {
  int firstIndex = 0;
  for (int i=0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() == 0) {
      firstIndex = i;
      break;
    }
  }

  resetLedValues();

  //add new burst of sparks
  if (sparkGap == 0 || millis() > sparkGap) {
    int newLedAmount = random(40,60);
    int newOrigin = (random(0, NUM_LEDS - 50) + 25 + lastSparkOrigin) % NUM_LEDS;
    lastSparkOrigin = newOrigin;
    
    for (int i = 0; i < newLedAmount ; i++) {
      if (firstIndex + i == NUM_SPARKS - 1) {
        break;
      }
      sparkArray[firstIndex + i]->resetSpark(newOrigin);
    }
    sparkGap = millis() + random(50,300);
  }
    // process sparks
    for (int i = 0; i < NUM_SPARKS; i++) {
      if (sparkArray[i]->getValue() > 0) {
        sparkArray[i]->iterate(NUM_LEDS);
        int ledIndex = sparkArray[i]->getLedIndex();
        ledValues[ledIndex] = addTwoFlameValues(ledValues[ledIndex], sparkArray[i]->getValue());  
      }
    }

    cleanUpSparks();
//    Serial.println(index);

    //display sparks
    for (int i = 0; i < NUM_LEDS; i++) {
      CRGB color = ColorFromPalette( sparkPalette, 255 - ledValues[i], 255);
      strip.setPixelColor((startLocation + i) % NUM_LEDS, color[0], color[1], color[2] );
    }
 }


void fireball(int startLocation, int numFireballs) {
  int firstIndex = 0;
  for (int i=0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() == 0) {
      firstIndex = i;
      break;
    }
  }

  resetLedValues();
  
  // add sparks
  int sparksToAdd = 2;
  
  // add fireball sparks
  for (int i = 0; i < NUM_SPARKS ; i++) {
    if(!sparkArray[i]->isAlive()) {
      int maxStrength = numFireballs == 1 ? 30 : 23;
      sparksToAdd--;
      int strength = random(0, maxStrength);
      float sSpeed = float(strength) / 10;
      if (isReversed) { sSpeed = -sSpeed; };
      sparkArray[i]->setSpark(135 + strength * 4, random(numFireballs) * (NUM_LEDS / numFireballs), sSpeed);
      if (sparksToAdd == 0) { break; }
    }
  }

  // process sparks
  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() > 0) {
      sparkArray[i]->iterateFireball(isReversed, NUM_LEDS);
      int ledIndex = sparkArray[i]->getLedIndex();
      for (int j = 0; j < 3; j ++) {
        int thisIndex = (ledIndex + j) % NUM_LEDS;
        ledValues[thisIndex] = addTwoFlameValues(ledValues[thisIndex], sparkArray[i]->getValue());        
      }  
    }
  }
  
//  cleanUpSparks();

//    Serial.print("Particle count: ");
//    Serial.println(index);

    //display sparks
  for (int i = 0; i < NUM_LEDS; i++) {
    CRGB color = ColorFromPalette( fireballPalette, 255 - ledValues[i], 255);
    strip.setPixelColor((startLocation + i) % NUM_LEDS, color[0], color[1], color[2] );
  }
}

void whiteToRgb(int startLocation, int rSpeed, int numLeds) {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledBallArray[i]->resetLedBall();
  }
  int width = 8;
  int shift = min(max(rSpeed / 100 - 2, 0), 12);
  int gap = numLeds / 3;
  for (int i = 0; i < 3; i++) {
    int startLed = i * gap;
    for(int j = 0; j < width; j++) {
      ledBallArray[j + startLed]->addLed(255,0,0);
    }
    for(int j = 0; j < width; j++) {
      ledBallArray[j + shift + startLed]->addLed(0,255,0);
    }
    for(int j = 0; j < width; j++) {
      ledBallArray[j + shift * 2 + startLed]->addLed(0,0,255);
    }
  }
  for (int i = 0; i < NUM_LEDS; i++) {
      int r = ledBallArray[i]->getRed();
      int g = ledBallArray[i]->getGreen();
      int b = ledBallArray[i]->getBlue();
      strip.setPixelColor((startLocation + i) % NUM_LEDS, r, g, b );
  }
}

int addTwoFlameValues(int flame1, int flame2) {
    int lower = min(flame1, flame2);
    int higher = max(flame1, flame2);
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

void resetReverse() {
  isReversed = false;
  speedFactor = 1.3;
}

void resetLedValues() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledValues[i] = 0;
  }
}

void cleanUpSparks() {
  int index = 0;
  for (int i = 0; i < NUM_SPARKS; i++) {
    if (sparkArray[i]->getValue() > 0) {
      sparkArray[index]->setSpark(sparkArray[i]->getValue(), sparkArray[i]->getLocation(), sparkArray[i]->getSpeed());
      if(i != index){
        sparkArray[i]->setSpark(0,0,0);
      }
      index++;
    }
  }
}

void resetSparks() {
  for (int i = 0; i < NUM_SPARKS; i++) {
    sparkArray[i]->killSpark();
  }
}
