#include <EEPROM.h>
#include "TinyMegaI2CMaster.h"
#include <tinyNeoPixel_Static.h>

#define LED_PIN 1
#define BUTTON_PIN 0
#define BRIGHTNESS_PIN 4

#define DEBUG true

#define MAX_BRIGHTNESS 150
//#define NUM_PIXELS 9
#define NUM_PIXELS 51

#define EEPROM_MODE_ADDRESS 0
#define EEPROM_COLOR_ADDRESS 1

byte pixels[NUM_PIXELS * 3];
tinyNeoPixel leds = tinyNeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB, pixels);

enum Mode {
  COLOR,
  GRADIENT,
  GRADIENT_ANIMATED,
  SPECTRUM,

  MODE_LENGTH // last value is the length of this enum
};

// Config options
uint8_t currentMode = GRADIENT_ANIMATED;
uint16_t currentColorHue = 0;
bool on = true;

uint8_t i;
uint8_t spectrum[16];

uint16_t cycle = 0;
uint8_t cycleCounter = 0;

uint16_t lastAnalog = 0;
uint8_t lastAmbient = 0;

void setup() {
  TinyMegaI2C.init();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);


  leds.setBrightness(10);
  if (DEBUG) {
    for (i = 0; i < NUM_PIXELS; i++) {
      leds.setPixelColor(i, leds.Color(0, 255, 0));
      delay(15);
      leds.show();
    }
  
    leds.show();
    delay(300);
    
    leds.fill(leds.Color(0, 0, 0));
    leds.show();
    delay(300);
  }

  EEPROM.get(EEPROM_MODE_ADDRESS, currentMode);
  currentMode = currentMode % MODE_LENGTH;
  
  EEPROM.get(EEPROM_COLOR_ADDRESS, currentColorHue);
  
  _PROTECTED_WRITE(WDT.CTRLA,WDT_PERIOD_512CLK_gc); //enable the WDT, with a 0.512s timeout (WARNING: delay > 0.5 will reset)
}

void updateSpectrum() {
  uint8_t amp = 25;

  // TODO: Rewrite w/o floats
//  float fadeFactor = 0.015;
  uint8_t fadeFactor = 15;

  uint16_t oldValue = 1000 - fadeFactor;
  uint16_t newValue = fadeFactor * amp;

//  float oldValue = 1 - fadeFactor;
//  float newValue = fadeFactor * amp;

  TinyMegaI2C.start(8, 16);

  for (i = 0; i < 16; i++) {
    spectrum[i] = max(100, (spectrum[i] * oldValue + TinyMegaI2C.read() * newValue) / 1000);
  }

  TinyMegaI2C.stop();
}

void checkButton() {
  uint16_t buttonValue = analogRead(BUTTON_PIN);

  if (abs(lastAnalog - buttonValue) > 20) { // Color
    if (buttonValue > 500) {
      on = true;
      currentColorHue += 6553;

      EEPROM.put(EEPROM_COLOR_ADDRESS, currentColorHue);
    } else if (buttonValue > 250) { // Mode
      on = true;
      cycle = 0;
      currentMode = (currentMode + 1) % MODE_LENGTH;
      
      EEPROM.put(EEPROM_MODE_ADDRESS, currentMode);
    } else if (buttonValue > 100) { // On / Off
      on = !on;
    }
  }

  lastAnalog = buttonValue;
}

void checkBrightness() {
  uint8_t ambient = analogRead(BRIGHTNESS_PIN) >> 4;
  if (ambient != lastAmbient) {
    leds.setBrightness(min(MAX_BRIGHTNESS, ambient));
    lastAmbient = ambient;
  }
}

void loop() {
  //  reset watchdog timer
  __asm__ __volatile__ ("wdr"::);

  checkButton();
  checkBrightness();

  if (on) {
    if (currentMode == COLOR) {
      leds.fill(leds.ColorHSV(currentColorHue, 255, 255));
    } else {
      if (currentMode == SPECTRUM) updateSpectrum();
      
      for (i = 0; i < NUM_PIXELS; i++) {
        uint16_t hue = map(i + cycle, 0, NUM_PIXELS * (currentMode == GRADIENT_ANIMATED ? 5 : 1), 0, 65535);
        uint16_t value = 255;
        
        if (currentMode == SPECTRUM) {
          uint8_t spectrumIndex = map(i, 0, NUM_PIXELS, 0, 16);
          value = spectrum[spectrumIndex];

//          hue = 0;
//          value = 255;
        }

        leds.setPixelColor(i, leds.ColorHSV(hue, 255, value));
      }
    }
  } else {
    leds.fill(leds.Color(0, 0, 0));
  }

  leds.show();

  if (on && currentMode == GRADIENT_ANIMATED) {
    cycleCounter++;

    if (cycleCounter > 5) {
      cycleCounter = 0;
      cycle++; 
    }
  }
  
  delay(15);
}
