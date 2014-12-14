// LED Ring
#include <FastLED.h> 
#include <Time.h>  

// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

// Timer Interrupt
#include <TimerOne.h>

#define NUM_LEDS 60
#define LED_BRIGHTNESS 150

#define FADE_BY(x) x*256/100 

#define FADE_HOUR2 FADE_BY(90)
#define FADE_HOUR1 FADE_BY(75)
#define FADE_MIN   FADE_BY(75)

#define LDR_PIN A0

#define T_SENSOR_PIN 2
#define RING_DATA_PIN 3

#define LED_CLOCK_PIN 5

#define LED_TEMP_OUT_PIN 6
#define LED_TEMP_LATCH_PIN 7

#define LED_DATE_OUT_PIN 8
#define LED_DATE_LATCH_PIN 9

CRGB leds[NUM_LEDS];
byte ledData[4];

int ldrIdx = 0;
int ldrValue[4] = {512,512,512,512};


byte digit[10] = {
  B11000000, // 0
  B11111001, // 1
  B10100100, // 2
  B10110000, // 3
  B10011001, // 4

  B10010010, // 5
  B10000010, // 6
  B11111000, // 7
  B10000000, // 8
  B10010000  // 9
}; 

#define DIGIT_EMPTY  B11111111
#define DIGIT_DOT    B01111111
#define DIGIT_MINUS  B10111111
#define DIGIT_GRAD   B10011100

OneWire oneWire(T_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

typedef struct {
  CRGB background;
  CRGB dots;
  CRGB hour[3];
  CRGB min[2];
  CRGB sec;
} ColorProfile;

ColorProfile cfg;

int isr_lastSec = -1;
unsigned long isr_milliSecOffset = 0;

volatile int curHour = 0;
volatile int curMin = 0;
volatile int curSec = 0;
volatile unsigned long curMillis = 0;

int lastDisplayAtSec = -1;

void setup() {
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  
  LEDS.addLeds<NEOPIXEL, RING_DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.setBrightness(LED_BRIGHTNESS);
  
  pinMode(LDR_PIN, INPUT);

  pinMode(LED_CLOCK_PIN, OUTPUT);
  pinMode(LED_TEMP_LATCH_PIN, OUTPUT);
  pinMode(LED_TEMP_OUT_PIN, OUTPUT);
  pinMode(LED_DATE_LATCH_PIN, OUTPUT);
  pinMode(LED_DATE_OUT_PIN, OUTPUT);

  sensors.begin();  // DS18B20 starten
  
  testLEDStrip();
  
  initColorProfile();
  
  Timer1.initialize(25000);
  Timer1.attachInterrupt(ISR_displayClock); 

  ledData[3] = DIGIT_MINUS;
  ledData[2] = DIGIT_MINUS & DIGIT_DOT;
  ledData[1] = DIGIT_MINUS;
  ledData[0] = DIGIT_MINUS & DIGIT_DOT;
  displayDate();

  ledData[3] = DIGIT_MINUS;
  ledData[2] = DIGIT_MINUS;
  ledData[1] = DIGIT_MINUS;
  ledData[0] = DIGIT_GRAD;
  displayTemperature();
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
  LEDS.setBrightness(LED_BRIGHTNESS); // reset brightness
}

void initColorProfile() {
  cfg.background = CRGB( 0, 0, 0);
  cfg.dots = CRGB(70,70,70);
  
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

void ISR_displayClock(void) {
  int isr_curMSec = 0;
  
  if (curSec > isr_lastSec || (curSec == 0 && isr_lastSec == 59)) {
    isr_lastSec = curSec;
    isr_milliSecOffset = curMillis;
  } else {
    isr_curMSec = curMillis - isr_milliSecOffset;
  }
  
  displayClock(curHour, curMin, curSec, isr_curMSec);
}  

void displayClock(int hour, int min, int sec, int msec) {
  // 1. set background
  for(int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = cfg.background; 
  }

  // 2. set dots
  for(int i = 0; i < NUM_LEDS; i += 5) {
    leds[i] = cfg.dots;
  }  
  
  int ledH = hour * 5 + (min / 12); 
  ledH = ledH > 55 ? ledH - 60 : ledH;
  
  setHourLeds(ledH);
  setMinLeds(min);  
  setSecLeds(sec, msec);
  
  LEDS.setBrightness(readBrightness());
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

void setSecLeds(int ledS, int msec) {
  CRGB tmp;
  int prop = (msec / 4);
  int iprop = 255 - prop;
  
  int idx = ledS - 1;
  if (idx < 0) idx += NUM_LEDS;
  
  tmp = cfg.sec;
  leds[idx] = tmp.fadeLightBy(prop) + leds[idx].fadeToBlackBy(iprop);

  ++idx;
  idx = idx == NUM_LEDS ? idx = 0 : idx;
  
  tmp = cfg.sec;
  leds[idx] = tmp.fadeLightBy(iprop) + leds[idx].fadeToBlackBy(prop);
}

int readBrightness() {
  ldrValue[ldrIdx] = analogRead(LDR_PIN);
  ldrIdx = (ldrIdx == 3) ? 0 : ldrIdx + 1;
  
  int sensorValue = (ldrValue[0] + ldrValue[1] + ldrValue[2] + ldrValue[3]) / 4; // average value 
  
  return sensorValue / 4; // adjust range 0..1024 -> 0..256
}

void loop() {
  curHour = hour();
  curMin = minute();
  curSec = second();
  curMillis = millis();

  if (curSec != lastDisplayAtSec) {
    lastDisplayAtSec = curSec;
    
    prepareTemperatureDisplay();
    displayTemperature();
    
    prepareDateDisplay();
    displayDate();
  }
}

void prepareDateDisplay() {
  byte data = day();
  ledData[3] = digit[data / 10];
  ledData[2] = digit[data % 10] & DIGIT_DOT;
  
  data = month();
  ledData[1] = digit[data / 10];
  ledData[0] = digit[data % 10] & DIGIT_DOT;
}

void displayDate() {
  digitalWrite(LED_DATE_LATCH_PIN, 0);
  shiftOut(LED_DATE_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[3]); 
  shiftOut(LED_DATE_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[2]); 
  shiftOut(LED_DATE_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[1]); 
  shiftOut(LED_DATE_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[0]); 
  digitalWrite(LED_DATE_LATCH_PIN, 1);
}

void prepareTemperatureDisplay() {
  sensors.requestTemperatures();                 // Temperatursensor auslesen
  float temp = sensors.getTempCByIndex(0);       // Temperatur ausgeben
  int data = (int)(temp * 10 + .5);
  
  int dig;
  if (data < 0) {
    data = abs(data);
    ledData[3] = DIGIT_MINUS;

    if (data > 99) {
      ledData[2] = digit[data / 100];
      ledData[1] = digit[(data % 100) / 10];
    } else {
      ledData[2] = digit[(data % 100) / 10] & DIGIT_DOT;
      ledData[1] = digit[data % 100 % 10];
    }
  } else {
    dig = data / 100; 
    ledData[3] = dig == 0 ? DIGIT_EMPTY : digit[dig];
    ledData[2] = digit[(data % 100) / 10] & DIGIT_DOT;
    ledData[1] = digit[data % 100 % 10];
  }
  ledData[0] = DIGIT_GRAD;
}

void displayTemperature() {
  digitalWrite(LED_TEMP_LATCH_PIN, 0);
  shiftOut(LED_TEMP_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[3]); 
  shiftOut(LED_TEMP_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[2]); 
  shiftOut(LED_TEMP_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[1]); 
  shiftOut(LED_TEMP_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[0]); 
  digitalWrite(LED_TEMP_LATCH_PIN, 1);
}


