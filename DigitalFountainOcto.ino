#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>

// Height measured in number of LEDs from bottom of tube to top
const int HEIGHTS[] = {50, 120, 120, 120, 120, 120, 120};

#define NUM_LEDS_PER_STRIP 240
#define NUM_STRIPS 7

#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP

CRGB leds[NUM_LEDS];

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(32);
  Serial.begin(9600);
}

#define CONFETTI_CONST 115
void loop() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 5);
  int buff = 120 - HEIGHTS[0];
  int pos = CONFETTI_CONST - sqrt16(random16(CONFETTI_CONST * CONFETTI_CONST));
  int brightness = 200;
  int hue = 128 + random8(64);// + 100 * getProximity(strip);
  if (pos < HEIGHTS[0]) {
    leds[buff + pos] += CHSV(hue, 200, brightness);
    leds[240 - (buff + pos)] += CHSV(hue, 200, brightness);
    //leds[numLeds - pos] += CHSV(hue, 200, brightness);
  }
  LEDS.show();
  delay(20);
}
