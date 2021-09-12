#include <EEPROM.h>
#include "TinyMegaI2CMaster.h"
#include <tinyNeoPixel_Static.h>

#define LED_PIN 1
#define BUTTON_PIN 0
#define BRIGHTNESS_PIN 4

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

const PROGMEM uint8_t solidColors[] = {
  0xFF, 0x00, 0x00, // Red
  0x00, 0xFF, 0x00, // Green
  0x00, 0x00, 0xFF, // Blue
  0xFF, 0xFF, 0x00, // Yellow
  0xFF, 0x00, 0xFF, // Purple
  0x00, 0xFF, 0xFF, // Cyan
};

// Config options
uint8_t currentMode = GRADIENT_ANIMATED;
uint8_t currentColorIndex = 0;
uint32_t currentColor;
bool on = true;

uint8_t i;
uint8_t spectrum[16];

uint16_t cycle = 0;
uint8_t cycleCounter = 0;

int lastAnalog = 0;
uint8_t lastAmbient = 0;

void setup() {
  TinyMegaI2C.init();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  leds.setBrightness(10);
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

  uint8_t eepromValue = EEPROM.read(EEPROM_MODE_ADDRESS);
  if (eepromValue < MODE_LENGTH) {
    currentMode = eepromValue;
  }

  eepromValue = EEPROM.read(EEPROM_COLOR_ADDRESS);
  if (eepromValue < sizeof(solidColors)) {
    currentColorIndex = eepromValue;
  }

  loadColor();
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
      currentColorIndex = (currentColorIndex + 3) % sizeof(solidColors);
      EEPROM.write(EEPROM_COLOR_ADDRESS, currentColorIndex);
      loadColor();
    } else if (buttonValue > 250) { // Mode
      on = true;
      cycle = 0;
      currentMode = (currentMode + 1) % MODE_LENGTH;
      EEPROM.write(EEPROM_MODE_ADDRESS, currentMode);
    } else if (buttonValue > 100) { // On / Off
      on = !on;
    }
  }

  lastAnalog = buttonValue;
}

void checkBrightness() {
  uint8_t ambient = analogRead(BRIGHTNESS_PIN) >> 4;
  if (ambient != lastAmbient) {
    leds.setBrightness(ambient);
    lastAmbient = ambient;
  }
}

void loadColor() {
  currentColor = leds.Color(pgm_read_byte_near(solidColors + currentColorIndex), pgm_read_byte_near(solidColors + currentColorIndex + 1), pgm_read_byte_near(solidColors + currentColorIndex + 2));
}

void loop() {
  //  reset watchdog timer
  __asm__ __volatile__ ("wdr"::);

  checkButton();
  checkBrightness();

  if (on) {
    if (currentMode == COLOR) {
      leds.fill(currentColor);
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
