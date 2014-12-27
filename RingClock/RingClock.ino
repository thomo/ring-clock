// LED Ring
#include <FastLED.h> 
#include <Time.h>  

// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

// read atomic time broadcast
#include <DCF77.h>

#define NUM_LEDS 60
#define LED_BRIGHTNESS 150

#define FADE_BY(x) x*256/100 

#define FADE_HOUR2 FADE_BY(90)
#define FADE_HOUR1 FADE_BY(75)
#define FADE_MIN   FADE_BY(75)

#define LDR_PIN A0

#define DCF_PIN 2
#define DCF_INTERRUPT 0

#define RING_DATA_PIN 3
#define T_SENSOR_PIN 4

#define LED_OUT_PIN 6
#define LED_CLOCK_PIN 7
#define LED_DATE_LATCH_PIN 8
#define LED_TEMP_LATCH_PIN 9

#define BLINK_PIN 13

CRGB leds[NUM_LEDS];
byte ledData[4];

int ldrIdx = 0;
int ldrValue[4] = {0,0,0,0};

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
#define DIGIT_r      B10101111
#define DIGIT_t      B10000111
#define DIGIT_c      B10100111
#define DIGIT_E      B10000110

OneWire oneWire(T_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
DCF77 DCF(DCF_PIN, DCF_INTERRUPT, true); 


typedef struct {
  CRGB background;
  CRGB dots;
  CRGB dotsQuarter;
  CRGB hour[3];
  CRGB min[2];
  CRGB sec;
} ColorProfile;

ColorProfile cfg;

int lastSec = -1;
unsigned long millisOffset = 0;

int curHour = 0;
int curMin = 0;
int curSec = 0;
unsigned long curMillis = 0;

int lastDigitUpdate = -1;
bool updateTime = false;

// max time without dcf sync
#define VALID_DCF_SYNC_TIME_LAPSE 60*24
int minutesSinceLastTimeUpdate = VALID_DCF_SYNC_TIME_LAPSE;

// === SETUP =========================================================================================================================
void setup() {
  pinMode(LDR_PIN, INPUT);
  pinMode(T_SENSOR_PIN, INPUT);

  pinMode(BLINK_PIN, OUTPUT);
  pinMode(RING_DATA_PIN, OUTPUT);
  pinMode(LED_CLOCK_PIN, OUTPUT);
  pinMode(LED_OUT_PIN, OUTPUT);
  pinMode(LED_TEMP_LATCH_PIN, OUTPUT);
  pinMode(LED_DATE_LATCH_PIN, OUTPUT);

  testDigits(DIGIT_EMPTY);
  
  DCF.Start();
  DCF.setSplitTime(105, 205);
  
  LEDS.addLeds<NEOPIXEL, RING_DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.setBrightness(LED_BRIGHTNESS);
  initColorProfile();
  
  testLEDStrip(CRGB::Red);
  testLEDStrip(CRGB::Green);
  testLEDStrip(CRGB::Blue);
  testLEDStrip(CRGB::White);
  
  testDigits();
  
  sensors.begin();  // DS18B20 starten
  sensors.requestTemperatures();
  sensors.setWaitForConversion(false);

  checkRTC();
  
  setTime(RTC.get());
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(10);
  
  prepareDateDisplay();
  displayDigits(LED_DATE_LATCH_PIN);

  prepareTemperatureDisplay();
  displayDigits(LED_TEMP_LATCH_PIN);
}

void testLEDStrip(CRGB c) {
  LEDS.setBrightness(100);
  for(int j = 0; j < 1; ++j) {
    for(int idx = 0; idx < NUM_LEDS; ++idx)
    {
      leds[idx] = c;
      LEDS.show();
      delay(15);
      leds[idx] = CRGB::Black;
    }
    LEDS.show();
  }
  LEDS.setBrightness(LED_BRIGHTNESS); // reset brightness
}

void testDigits() {
    int d = 200;
    testDigits(B11111110);
    delay(d);
    testDigits(B11111101);
    delay(d);
    testDigits(B10111111);
    delay(d);
    testDigits(B11101111);
    delay(d);
    testDigits(B11110111);
    delay(d);
    testDigits(B11111011);
    delay(d);
    testDigits(B10111111);
    delay(d);
    testDigits(B11011111);
    delay(d);
    testDigits(B01111111);
    delay(d);
    testDigits(DIGIT_EMPTY);
}

void testDigits(int value) {
  for(int i = 0; i < 4; ++i) {
    ledData[i] = value;
  }
  displayDigits(LED_DATE_LATCH_PIN);
  displayDigits(LED_TEMP_LATCH_PIN);
}

void initColorProfile() {
  cfg.background = CRGB( 0, 0, 0);
  cfg.dots = CRGB::DarkGray;
  cfg.dots.fadeToBlackBy(192);
  cfg.dotsQuarter = CRGB::DarkCyan;
  cfg.dotsQuarter.fadeToBlackBy(192);
  
  cfg.hour[0] = CRGB::Blue;
  cfg.hour[1] = cfg.hour[0];
  cfg.hour[1].fadeLightBy(FADE_HOUR1);
  cfg.hour[2] = cfg.hour[0];
  cfg.hour[2].fadeLightBy(FADE_HOUR2);
  
  cfg.min[0]  = CRGB::Green;
  cfg.min[1]  = cfg.min[0];
  cfg.min[1].fadeLightBy(FADE_MIN);;
  
  cfg.sec  = CRGB::Red;
}

void checkRTC() {
  tmElements_t tm;
  int cnt = 0;
  bool isReady = false;
  
  while(!isReady) {
    if (RTC.read(tm)) {
      isReady = true;
    } else {
      if (RTC.chipPresent()) {
        tm.Second = 0;
        tm.Minute = 0;
        tm.Hour = 0;
        tm.Wday = 5;
        tm.Day = 1;
        tm.Month = 1;
        tm.Year = 2015;
        RTC.write(tm);
      } else {
        ledData[3] = DIGIT_r;
        ledData[2] = DIGIT_t;
        ledData[1] = DIGIT_c;
        ledData[0] = DIGIT_EMPTY;
        displayDigits(LED_DATE_LATCH_PIN);
        ledData[3] = DIGIT_E;
        ledData[2] = DIGIT_r;
        ledData[1] = DIGIT_r;
        ledData[0] = DIGIT_EMPTY;
        displayDigits(LED_TEMP_LATCH_PIN);
        
        ++cnt;
        cnt %= NUM_LEDS;
        
        for(int i = 0; i < NUM_LEDS; ++i) {
          leds[i] = (i == cnt) ? CRGB::Red : CRGB::Black;
        }
        LEDS.show();
      }
      delay(1000);
    }
  }
}

// === LOOP =========================================================================================================================
void loop() {
  time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
  if (DCFtime!=0) {
    RTC.set(DCFtime);
    setTime(DCFtime);
    updateTime = true;
  }     
  
  if (DCF.isPulse()) {
    digitalWrite(BLINK_PIN, HIGH);
  } else {
    digitalWrite(BLINK_PIN, LOW);
  }
  
  curHour = hour();
  curMin = minute();
  curSec = second();
  curMillis = millis();
  
  displayClock();

  readBrightness();    

  if (curSec != lastDigitUpdate) {
    lastDigitUpdate = curSec;

    if (curSec == 0) {
      if (updateTime) {
        minutesSinceLastTimeUpdate = 0;
      } else {
        ++minutesSinceLastTimeUpdate; 
      }
      updateTime = false;
    }
    
    if (curSec % 10 == 0) {
      prepareTemperatureDisplay();
      displayDigits(LED_TEMP_LATCH_PIN);
    }
    
    if (curSec % 30 == 0) {
      prepareDateDisplay();
      displayDigits(LED_DATE_LATCH_PIN);
    }
  }
  
  delay(50);
}

// === CLOCK DISPLAY =========================================================================================================================

void displayClock(void) {
  int curMSec = 0;
  
  if (curSec == lastSec) {
    curMSec = curMillis - millisOffset;
  } else {
    lastSec = curSec;
    millisOffset = curMillis;
  }

  // display clock layout
  
  // 1. set background
  for(int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = cfg.background; 
  }

  // 2. set dots
  for(int i = 0; i < NUM_LEDS; i += 5) {
    leds[i] = cfg.dots;
    if ((i % 15) == 0) {
      leds[i] = cfg.dotsQuarter;
    }
  }  
  
  int ledH = curHour * 5 + (curMin / 12); 
  ledH = ledH > 55 ? ledH - 60 : ledH;
  
  setHourLeds(ledH);
  setMinLeds(curMin);  
  setSecLeds(curSec, curMSec);
  
  LEDS.setBrightness(calcBrightness());
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
  msec = (msec > 1024) ? 1024 :  msec;
  int prop = (msec / 4);
  int iprop = 255 - prop;

  setSecLed(ledS, prop, iprop);
  setSecLed((ledS == NUM_LEDS - 1) ? 0 : ledS + 1, iprop, prop);
}

void setSecLed(int idx, int prop, int iprop) {
  CRGB tmp = cfg.sec;
  leds[idx] = tmp.fadeLightBy(prop) + leds[idx].fadeToBlackBy(iprop);
}

void readBrightness() {
  ldrValue[ldrIdx] = abs(analogRead(LDR_PIN) - 1024) + 15;
  if (ldrValue[ldrIdx] > 512) ldrValue[ldrIdx] = 512;
  ldrIdx = (ldrIdx == 3) ? 0 : ldrIdx + 1;
}

int calcBrightness() {  
  int sensorValue = (ldrValue[0] + ldrValue[1] + ldrValue[2] + ldrValue[3]) / 4; // average value 
  return sensorValue / 2; // adjust range 0..512 -> 0..256
}

// === DATE + TEMPERATURE =============================================================================================================

void prepareDateDisplay() {
  byte data = day();
  ledData[3] = digit[data / 10];
  ledData[2] = digit[data % 10] & DIGIT_DOT;
  
  data = month();
  ledData[1] = digit[data / 10];
  ledData[0] = digit[data % 10] & DIGIT_DOT;
}

void prepareTemperatureDisplay() {
  float temp = sensors.getTempCByIndex(0);       // get temperature
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

  // show dcf sync marker, but only if last sync happens in last VALID_DCF_SYNC_TIME_LAPSE minutes
  if (minutesSinceLastTimeUpdate < VALID_DCF_SYNC_TIME_LAPSE) {
    ledData[0] &= DIGIT_DOT;
  }

  sensors.requestTemperatures();                 // request next measurement
}

void displayDigits(int latchPin) {
  digitalWrite(latchPin, 0);
  shiftOut(LED_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[3]); 
  shiftOut(LED_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[2]); 
  shiftOut(LED_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[1]); 
  shiftOut(LED_OUT_PIN, LED_CLOCK_PIN, MSBFIRST, ledData[0]); 
  digitalWrite(latchPin, 1);
}


