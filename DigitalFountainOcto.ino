#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>
#include "RunningMedian.h"

// Height measured in number of LEDs from bottom of tube to top
const int HEIGHTS[] = {60, 60, 120, 120, 120, 120, 120};
#define MAX_HEIGHT 115

#define NUM_LEDS_PER_STRIP 240
#define NUM_STRIPS 7

#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP

CRGB leds[NUM_LEDS];
#define MODE_DURATION 20 // in seconds
#define FRAMES_PER_SECOND 60
#define NUM_FRAMES MODE_DURATION * FRAMES_PER_SECOND
#define REFRESH_DELAY (1000/FRAMES_PER_SECOND) // time delay between LED changes (in milliseconds)
#define NUM_MODES 3
#define MAX_BRIGHTNESS 255

// IR sensors
#define NUM_IR 2
#define MAX_DISTANCE 400 // max distance that we care about
const int IR_PINS[] = {22, 19}; //21 doesn't work

int lastSensorValue[NUM_IR];
RunningMedian values1 = RunningMedian(8);
RunningMedian values2 = RunningMedian(8);

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(255); // overall brightness
  Serial.begin(9600);
}

int modeIndex = 0;
void loop() {
  for (int t = 0; t < NUM_FRAMES; t++) {
    if (modeIndex == 0) {
      confetti(t);
    } else if (modeIndex == 1) {
      sineDrips(t);
    } else if (modeIndex == 2) {
      fadeSine(t);
    }
    LEDS.show();
    delay(REFRESH_DELAY);
    //Serial.println(t);
  }

  delay(1000);
  // get next mode index
  nextMode();
}

void nextMode() {
  if (modeIndex == NUM_MODES - 1) {
    modeIndex = 0;
  } else {
    modeIndex++;
  }
}

void clearLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].setHSV(0, 0, 0);
  }
}

const int fadeBy = 5;
void confetti(int t) {
  fadeToBlackBy(leds, NUM_LEDS, fadeBy);
  if (t > (NUM_FRAMES - (MAX_BRIGHTNESS / fadeBy) - map(MAX_BRIGHTNESS, 0, 255, 0, 77))) {
    return;
  }
  for (int strip = 0; strip < NUM_STRIPS; strip++) {
    // random colored speckles that blink in and fade smoothly
    int buff = getMidpoint(strip) - HEIGHTS[strip];
    int stripPos = NUM_LEDS_PER_STRIP * strip;
    int pos = MAX_HEIGHT - sqrt16(random16(MAX_HEIGHT * MAX_HEIGHT));
    int brightness = getBright(t, MAX_BRIGHTNESS);
    int hue = 135 + random8(64) + 100 * (1 - getProximity(strip));
    if (pos < HEIGHTS[strip]) {
      leds[buff + pos + stripPos] += CHSV(hue, 200, brightness);
      leds[getMidpoint(strip) * 2 - (buff + pos) + stripPos] += CHSV(hue, 200, brightness);
      //leds[numLeds - pos] += CHSV(hue, 200, brightness);
    }
  }
}

void sineDrips(int t) {
  for (int strip = 0; strip < NUM_STRIPS; strip++) {
    int angle = (5 * t) % 256;
    int hue = 230 + (1 - getProximity(strip)) * 100;
    int stripPos = (strip * NUM_LEDS_PER_STRIP);

    for (int i = getMidpoint(strip) - HEIGHTS[strip]; i < getMidpoint(strip); i++) { /////
      int brightness = getBright(t, map((sin8((angle + i * 10))), 0, 255, 50, 200));
      leds[stripPos + i] = CHSV(hue - i * .8, 255, brightness);
      leds[stripPos + getMidpoint(strip) * 2 - 1 - i] = CHSV(hue - i * .8, 255, brightness);
    }
  }
}

void fadeSine(int t) {
  clearLEDs();
  // Constanst for fade sine
  int AMPLITUDE = 10;       // Amplitude of sin wave
  int FADE = 20;            // Num pins over which wade fades out
  int PERIOD = 1;           // Seconds for one cycle to complete
  int MIN_BRIGHTNESS = 0;  // Brightness: 0-255
  int hue = 225;            // blue
  for (int strip = 0; strip < NUM_STRIPS; strip++) {
    int BASE = 80 * (1 - getProximity(strip));        // Num permanently on pins
    int stripPos = (strip * NUM_LEDS_PER_STRIP);
    // find length of full brightness portion
    float l = BASE + AMPLITUDE + AMPLITUDE * (sin8( (255 * t) / (PERIOD * FRAMES_PER_SECOND) ) / 126.0 - 1);
    for (int i = 0; i < l; i++) {
      int brightness = getBright(t, MAX_BRIGHTNESS);
      leds[stripPos + map(i, 0, 120, getMidpoint(strip) - HEIGHTS[strip], getMidpoint(strip))].setHSV(hue - i * .8, 255, brightness);
      leds[stripPos + getMidpoint(strip) * 2 - 1 - map(i, 0, 120, getMidpoint(strip) - HEIGHTS[strip], getMidpoint(strip))].setHSV(hue - i * .8, 255, brightness);
    }

    // Fade ends of wave out
    for (int i = 1; i < FADE; i++) {
      int brightness = getBright(t, (float)(FADE - i + (l - (int)l)) / (FADE) * (MAX_BRIGHTNESS - MIN_BRIGHTNESS) + MIN_BRIGHTNESS);
      int newL = int(l); // Make a version of l as an integer for accessing list items
      leds[stripPos + map(newL + i, 0, 120, getMidpoint(strip) - HEIGHTS[strip], getMidpoint(strip))].setHSV(hue - (i + newL)*.8, 255, brightness);
      leds[stripPos + getMidpoint(strip) * 2 - 1 - map(newL + i, 0, 120, getMidpoint(strip) - HEIGHTS[strip], getMidpoint(strip))].setHSV(hue - (i + newL)*.8, 255, brightness);
    }
  }


}

/*** HELPER FUNCTIONS ***/
int getBright(int t, int maxBrightness) {
  if (t < 0.5 * FRAMES_PER_SECOND) {
    return map(t, 0, 0.5 * FRAMES_PER_SECOND, 0, maxBrightness);
  } else if (t > (MODE_DURATION - 0.5)*FRAMES_PER_SECOND) {
    return map(t, (MODE_DURATION - 0.5) * FRAMES_PER_SECOND, MODE_DURATION * FRAMES_PER_SECOND - 1, maxBrightness, 0);
  } else {
    return maxBrightness;
  }
}

int getMidpoint(int strip) {
  if (strip == 0) { // i.e. pin 4
    return 84;
  }
  return 120;
}

float getProximity(int strip) {
  //return 0; // ignore IR
  //  Serial.print("Raw Input: ");
  //  Serial.print(x);
  //  Serial.print("   Mapped Output: ");
  //  Serial.print(distance);
  //  Serial.print("cm");
  //  Serial.print("   Value: ");
  //  Serial.println(value);

  int index = strip % 2; //strip; // change if needed
  int IRPin = IR_PINS[index];
  if (index == 0) {
    lastSensorValue[index] = readIR(IRPin, index, values1);
  } else if (index == 1) {
    lastSensorValue[index] = readIR(IRPin, index, values2);
  }
  //  Serial.print(index);
  //  Serial.print("\t");
  //  Serial.println(lastSensorValue[index]);
  return (lastSensorValue[index]) / 100.0;
}

// Returns a value 0 (100cm away) to 100 (350cm away)
int readIR(int IRPin, int index, RunningMedian values) {

  // Convert from sensor input to range 1-100
  int x = analogRead(IRPin);
  float volts = map(x, 0, 1023, 0, 500);
  volts /= 100.0;
  if (volts < 1.2) {
    volts = 1.2; // prevents division by zero/negative values
  }
  float calibration = 450.0;
  float distance = calibration / (volts - 1.1); // Volts = 138/Length + 1.1 from graph
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

    Serial.print(index); 
    Serial.print("Raw Input: ");
    Serial.print(x);
    Serial.print("\tMapped Output: ");
    Serial.print(distance);
    Serial.print("cm");
    Serial.print("\tVolts: ");
    Serial.print(volts);
    Serial.print("\tValue1: ");
    Serial.print(value);
    Serial.print("\tlastValue: ");
    Serial.print(lastValue);
    Serial.print("\toutputValue: ");
    Serial.println(outputValue);
  
  return outputValue;
}
