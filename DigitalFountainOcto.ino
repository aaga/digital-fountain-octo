#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>
#include "RunningMedian.h"

// Height measured in number of LEDs from bottom of tube to top
const int HEIGHTS[] = {115, 50, 120, 120, 120, 120, 120};
#define MAX_HEIGHT 115

#define NUM_LEDS_PER_STRIP 240
#define NUM_STRIPS 7

#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP

CRGB leds[NUM_LEDS];
#define MODE_DURATION 5 // in seconds
#define FRAMES_PER_SECOND 30
#define NUM_FRAMES MODE_DURATION * FRAMES_PER_SECOND
#define REFRESH_DELAY (1000/FRAMES_PER_SECOND) // time delay between LED changes (in milliseconds)
#define NUM_MODES 1
#define MAX_BRIGHTNESS 100

// IR sensors
#define NUM_IR 2
#define MAX_DISTANCE 250 // max distance that we care about
const int IR_PINS[] = {18, 19};

int lastSensorValue[NUM_IR];
RunningMedian values1 = RunningMedian(8);
RunningMedian values2 = RunningMedian(8);

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(32);
  Serial.begin(9600);
}

int modeIndex = 1;
void loop() {
  for (int t = 0; t < NUM_FRAMES; t++) {
    if (modeIndex == 0) {
      confetti(t);
    } else if (modeIndex == 1) {
      sineDrips(t);
    }// else if (modeIndex == 2) {
//      sineDripsProximity(t);
//    } else if (modeIndex == 3) {
//      fadeSine(t);
//    }
    LEDS.show();
    delay(REFRESH_DELAY);
    //Serial.println(t);
  }
  
  delay(1000);
  // get next mode index
//  if (modeIndex == NUM_MODES - 1) {
//    modeIndex = 0;
//  } else {
//    modeIndex++;
//  }
  LEDS.show();
  delay(20);
}

const int fadeBy = 5;
void confetti(int t) {
  fadeToBlackBy(leds, NUM_LEDS, fadeBy);
  if (t > (NUM_FRAMES - (MAX_BRIGHTNESS/fadeBy) - 15)) {
    Serial.println(t);
    return;
  }
  for (int strip = 0; strip < NUM_STRIPS; strip++) {
    // random colored speckles that blink in and fade smoothly
    int buff = 120 - HEIGHTS[strip];
    int stripPos = NUM_LEDS_PER_STRIP * strip;
    int pos = MAX_HEIGHT - sqrt16(random16(MAX_HEIGHT * MAX_HEIGHT));
    int brightness = getBright(t, MAX_BRIGHTNESS);
    int hue = 135 + random8(64) + 100 * getProximity(strip);
    if (pos < HEIGHTS[strip]) {
      leds[buff + pos + stripPos] += CHSV(hue, 200, brightness);
      leds[NUM_LEDS_PER_STRIP - (buff + pos) + stripPos] += CHSV(hue, 200, brightness);
      //leds[numLeds - pos] += CHSV(hue, 200, brightness);
    }
  }
}

void sineDrips(int t) {
  for(int strip = 0; strip < NUM_STRIPS; strip++) {
    int angle = (5*t)%256;
    int hue = 210 + getProximity(strip)*100;
    for(int i = 120 - HEIGHTS[strip]; i < 120; i++) {
      int brightness = map((sin8((angle + i*10))), 0, 255, 35, 200);
      leds[(strip*NUM_LEDS_PER_STRIP) + i] = CHSV(hue - i*.6, 255, brightness);
      leds[(strip*NUM_LEDS_PER_STRIP) + 239 - i] = CHSV(hue - i*.6, 255, brightness);
    }
  }
}

/*** HELPER FUNCTIONS ***/
int getBright(int t, int maxBrightness) {
  if (t < 0.5*FRAMES_PER_SECOND) {
    return map(t, 0, 0.5*FRAMES_PER_SECOND, 0, maxBrightness);
  } else if (t > (MODE_DURATION - 0.5)*FRAMES_PER_SECOND) {
    return map(t, (MODE_DURATION - 0.5)*FRAMES_PER_SECOND, MODE_DURATION*FRAMES_PER_SECOND - 1, maxBrightness, 0);
  } else {
    return maxBrightness;
  }
}

float getProximity(int strip) {
  return 0;
//  Serial.print("Raw Input: ");
//  Serial.print(x);
//  Serial.print("   Mapped Output: ");
//  Serial.print(distance);
//  Serial.print("cm");
//  Serial.print("   Value: ");
//  Serial.println(value);

  int index = strip; // change if needed
  int IRPin = IR_PINS[index];
  if (index == 0) {
    lastSensorValue[index] = readIR(IRPin, index, values1);
  } else if (index == 1) {
    lastSensorValue[index] = readIR(IRPin, index, values2);
  }
  return (lastSensorValue[index])/100.0;  
}

// Returns a value 0 (100cm away) to 100 (350cm away)
int readIR(int IRPin, int index, RunningMedian values) {
  
  // Convert from sensor input to range 1-100
  int x = analogRead(IRPin);
  Serial.println(x);
  float volts = map(x,0,1023,0,500);
  volts /= 100.0;
  float distance = 138.0/(volts-1.1); // Volts = 138/Length + 1.1 from graph
  int newValue = map(distance, 100, MAX_DISTANCE, 0, 100);

  // Cap distance
  if (newValue > 100) newValue = 100;
  if (newValue < 0)   newValue = 0;

  // Calculate average of inner 4 values
  values.add(newValue);
  float median = values.getAverage(4);
  int value = median;
  
  // Output value at most one different from last value
  int outputValue;
  int lastValue = lastSensorValue[index];
  if (value > lastValue) {
    outputValue = (lastValue + 1);
  } else if (value < lastValue) {
    outputValue = (lastValue - 1);
  } else {
    outputValue = (lastValue);
  }

//  Serial.print("Raw Input: ");
//  Serial.print(x);
//  Serial.print("\tMapped Output: ");
//  Serial.print(distance);
//  Serial.print("cm");
//  Serial.print("\tValue1: ");
//  Serial.print(value);
//  Serial.print("\tlastValue: ");
//  Serial.print(lastValue);
//  Serial.print("\toutputValue: ");
//  Serial.println(outputValue);
  
  return outputValue;
}
