
#include <Arduino_LSM6DS3.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include "Spark.h"
#include "LedBall.h"

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    7

// How many NeoPixels are attached to the Arduino?
#define NUM_LEDS 104
#define NUM_SPARKS 100
#define MESSAGE_LENGTH 3

#define BRIGHTNESS 50

// Number of frames per second
#define FREQUENCY 200
// determines how far the starting point moves depending on the speed in degrees per second
float frequencyRatio = float(NUM_LEDS) / (360 * FREQUENCY);

int maxSpeed = 400;

// speed and location variables when moving
float currentSpeed = 0;
int startLocation = 0;
float startLocationFloat = .000001;

// circle constants
const float circleRadius = float(NUM_LEDS) / PI;
const float angle = 2 * PI / NUM_LEDS;
float xOffsets[NUM_LEDS];
float yOffsets[NUM_LEDS];

//noise constants
const int speedHue = 3;   //Hue value is 16-bit
const int scaleHue = 10;
const int speedSat = 6;
const int scaleSat = 30;
const int speedBright = 7;
const int scaleBright = 10;

String mode = "none"; // options 1: normal, 2: complementary, 3: triad

// noise variables
float x = 0.0;
float y = 0.0;
float zHue = 100;
float zSat = 50;
float zBright = 0;

// color variables
float colorStart = 0;
float colorRange = .7;  //Range of each section of color 1 = 100%
float colorSpeed = 80;  //Speed of color cycling
int colorMax = 65536;

//spark variables
int ledValues[NUM_LEDS];
int ledPalettes[NUM_LEDS];
Spark *sparkArray[NUM_SPARKS];
LedBall *ledBallArray[NUM_LEDS];
int sparkGap = 0;
int lastSparkOrigin = 0;
int sparkCount = 0;
int ballTimer = 0;

// moving state management
boolean moving = true;
boolean phasing = false;
int phaseTimer = 0;
int selectionTimer = 0;

// palette variable
int displayMode = 0;
int paletteId = 0;
int sparkId = 0;
int fireballId = 0;
int debugMode = 0;
boolean isReversed = false;
float speedFactor = 1.3;
int paletteRGBValues[NUM_LEDS * 3];

//incoming data
int nextDisplayMode = 0;
int nextSubMode = 0;
char incomingChars[8];
int incomingIdx = 0;

int frameCounter = 0;

// the possible states of the state-machine
typedef enum {  NONE, GOT_M, GOT_S, GOT_X } states;

// current state-machine state
states state = NONE;
// current partial number
unsigned int currentValue;

// testing variable
int timecount = 0;
int lastTime = 0;
int thisTime = 0;
int timeGap = 0;

TBlendType    currentBlending;

extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const TProgmemRGBGradientPalettePtr gFireballPalettes[];
extern const TProgmemRGBGradientPalettePtr gSparkPalettes[];

extern const uint8_t gGradientPaletteCount;
extern const uint8_t gFireballPaletteCount;
extern const uint8_t gSparkPaletteCount;

CRGBPalette256 currentPalette( gGradientPalettes[paletteId] );
CRGBPalette256 fireballPalette(gFireballPalettes[fireballId]);
CRGBPalette256 sparkPalette(gSparkPalettes[sparkId]);

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);   // Serial for USB
  Serial1.begin(9600);  // Serial for Rx - bluetooth
  
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("Gyroscope sample rate = ");
  Serial.println(IMU.gyroscopeSampleRate());

  strip.begin();           // INITIALIZE NeoPixel strip object
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);

  // set up LED locations in circle
  for (int i = 0; i < NUM_LEDS; i++)
  {
    float thisAngle = angle * i;
    float xoffset = cos(thisAngle) * circleRadius;
    float yoffset = sin(thisAngle) * circleRadius;
    xOffsets[i] = xoffset;
    yOffsets[i] = yoffset;
  }

  for (int i = 0; i < NUM_SPARKS; i++) {
    sparkArray[i] = new Spark(0, 0);
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    ledValues[i] = 0;
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    ledBallArray[i] = new LedBall();
  }
  currentBlending = LINEARBLEND;
  loadPaletteToArray(1);

  lastTime = millis();
}

void loop() {
  float xIMU, yIMU, zIMU;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(xIMU, yIMU, zIMU);
    currentSpeed = abs(zIMU) * speedFactor;
  }

  if (currentSpeed <= 10) { currentSpeed = 0; }
  
  if (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
    ProcessIncomingChar(Serial1.read());
  }

  if (debugMode == 2) {
    startLocation = 0;
  }
  else {
  startLocationFloat += frequencyRatio * currentSpeed;
  if (startLocationFloat > NUM_LEDS) {
    startLocationFloat -= NUM_LEDS;
  };
 
  startLocation = (int) startLocationFloat;
  } 

  strip.clear();

  if (currentSpeed > 60 && !moving) {
    moving = true;
    phaseTimer = millis();
    phasing = true;
  }
  if (currentSpeed < 40 && moving) {
    moving = false;
    phaseTimer = millis();
    phasing = true;
  }

  if (phasing && millis() >= phaseTimer + 1000) {
    phasing = false;
  }

  if (phasing) {
    int counter = millis() - phaseTimer;
    if (counter < 500) {
      if (moving) {
        fillNoiseFade(counter);
      }
      else {
        paletteFade(startLocation, counter);
      }
    }
    else {
      if (moving) {
        if  (displayMode == 1) {
          paletteFade(startLocation, counter);          
        }
      }
      else {
        fillNoiseFade(counter);
      }
    }
  }
  else if (moving || millis() < selectionTimer) {
    if (displayMode == 0 ) {
      palette(startLocation);      
      delay(2); // getting closer to 200fps from natural 400 fps for Arduino Nano 33 IOT
    }
    if (displayMode == 1 ) {
      fireball(startLocation, 1);
    }
    if (displayMode == 2 ) {
      sparks(startLocation);
    }
    if (displayMode == 3 ) {
      fireball(startLocation, 2);
    }
    
//        colorBalls(startLocation);
//        whiteToRgb(startLocation, currentSpeed, NUM_LEDS);
  
  }
  else {
    fillnoise(1);
  }
  if (debugMode == 1) {
    referenceBar(5);
  }

    strip.show();                          //  Update strip to match    
//
//    if (pauseFastLedTimer < millis()) {
//    }
//  

//  frameCounter++;
//  if (frameCounter == 500) {
//    frameCounter = 0;
//    int timeMeasure = millis() - lastTime;
//    Serial.print((500*1000) / timeMeasure);
//    Serial.println(" frames per second");
//    lastTime = millis();
//  }
    
}

void updateWheel(int mode, int subMode, boolean reversed) {
  Serial.print("Updating mode: ");
  Serial.print(mode);
  Serial.print(" submode: ");
  Serial.print(subMode);
  Serial.print(" reversed: ");
  Serial.println(reversed ? "reversed" : "not-reversed");
  
  displayMode = mode;
  
  if (mode == 0) {
    paletteId = subMode % gGradientPaletteCount;
    currentPalette = gGradientPalettes[ paletteId ];
    loadPaletteToArray(1);
    if (reversed ) speedFactor = 1.9;
    if (!reversed) speedFactor = 1.3;
  }
  if (mode == 1 || mode == 3 ) {
    fireballId = subMode % gFireballPaletteCount;
    fireballPalette = gFireballPalettes[ fireballId ];
    if (reversed ) speedFactor = 2.1;
    if (!reversed) speedFactor = 1.5;  
  }
  if (mode == 2) {
    sparkId = subMode % gSparkPaletteCount;
    sparkPalette = gSparkPalettes[ sparkId ];
    if (reversed ) speedFactor = 2.1;
    if (!reversed) speedFactor = 1.5;
  }

  isReversed = reversed;
  selectionTimer = millis() + 1000;
}
