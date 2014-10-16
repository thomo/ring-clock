#include <FastLED.h> 
#include <Time.h>  

#define NUM_LEDS 60
#define LED_BRIGHTNESS 150

#define FADE_BY(x) x*256/100 

#define FADE_HOUR2 FADE_BY(90)
#define FADE_HOUR1 FADE_BY(75)
#define FADE_MIN   FADE_BY(75)

#define LED_DATA_PIN 3

CRGB leds[NUM_LEDS];

typedef struct {
  CRGB background;
  CRGB dots;
  CRGB hour[3];
  CRGB min[2];
  CRGB sec;
} ColorProfile;

ColorProfile cfg;

unsigned long milliSecOffset = 0;
int lastSec = -1;

void setup() {
  Serial.begin(9600); 
  
  LEDS.addLeds<NEOPIXEL, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.setBrightness(LED_BRIGHTNESS);
  
  testLEDStrip();
  
  initColorProfile();
  
  setTime(9, 17, 0, 10,10,14);
}

void loop() {
  displayClock();
  
}

void testLEDStrip() {
  LEDS.setBrightness(100);
  for(int j = 0; j < 1; ++j) {
    for(int idx = 0; idx < NUM_LEDS; ++idx)
    {
      leds[idx] = CRGB::White;
      LEDS.show();
      delay(10);
      leds[idx] = CRGB::Black;
    }
    LEDS.show();
  }
  LEDS.setBrightness(LED_BRIGHTNESS);//reset brightness
}

void initColorProfile() {
  cfg.background = CRGB( 0, 0, 0);
  cfg.dots = CRGB(80,80,80);
  
  cfg.hour[0] = CRGB::Blue;
  cfg.hour[1] = cfg.hour[0];
  cfg.hour[1].fadeLightBy(FADE_HOUR1);
  cfg.hour[2] = cfg.hour[0];
  cfg.hour[2].fadeLightBy(FADE_HOUR2);
  
  cfg.min[0]  = CRGB::Green;
  cfg.min[1]  = cfg.min[0];
  cfg.min[1].fadeLightBy(FADE_MIN);;
  
  cfg.sec  = CRGB::IndianRed;
}

void displayClock() {
  for(int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = cfg.background; // background first
  }
  
  for(int i = 0; i< NUM_LEDS; i += 5) {
    leds[i] = cfg.dots;
  }  
  
  int ledH = hour() * 5;
  int ledM  = minute();
  int ledS  = second();
  int msec = millis();
    
  if (ledS > lastSec || (ledS == 0 && lastSec == 59)) {
    lastSec = ledS;
    milliSecOffset = msec;
  }
  
  ledH += (ledM / 12); 
  ledH = ledH > 55 ? ledH - 60 : ledH;
  
  setHourLeds(ledH);
  setMinLeds(ledM);  
  setSecLeds(ledS, (msec - milliSecOffset) / 4);
  
  LEDS.show();
}

void setHourLeds(int ledH) {
  int idx = ledH - 2;
  if (idx < 0) idx += NUM_LEDS;
  
  leds[idx] = cfg.hour[2];
  
  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;

  leds[idx] = cfg.hour[1];

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;

  leds[idx] = cfg.hour[0];

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;

  leds[idx] = cfg.hour[1];

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;
  
  leds[idx] = cfg.hour[2];
}

void setMinLeds(int ledM) {
  int idx = ledM - 1;
  if (idx < 0) idx += NUM_LEDS;
  
  leds[idx] = cfg.min[1] + leds[idx].fadeToBlackBy(180);
  
  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;

  leds[idx] = cfg.min[0] + leds[idx].fadeToBlackBy(220);

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;

  leds[idx] = cfg.min[1] + leds[idx].fadeToBlackBy(180);
}

void setSecLeds(int ledS, int prop) {
  CRGB tmp;
  int iprop = 255-prop;
  
  int idx = ledS - 1;
  if (idx < 0) idx += NUM_LEDS;
  
  tmp = cfg.sec;
  leds[idx] = tmp.fadeLightBy(prop) + leds[idx].fadeToBlackBy(iprop);

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;
  
  tmp = cfg.sec;
  leds[idx] = tmp.fadeLightBy(iprop) + leds[idx].fadeToBlackBy(prop);
}

