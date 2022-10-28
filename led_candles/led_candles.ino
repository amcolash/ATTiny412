#include "ClickButton.h"
#include <EEPROM.h>
#include <tinyNeoPixel_Static.h>

#define LED1_PIN PIN_PA7
#define LED2_PIN PIN_PA6

#define BUTTON_PIN PIN_PA1

#define NUMPIXELS_1 6
#define NUMPIXELS_2 6

#define EEPROM_ADDRESS 0

byte pixels1[NUMPIXELS_1 * 3];
byte pixels2[NUMPIXELS_2 * 3];

tinyNeoPixel leds1 = tinyNeoPixel(NUMPIXELS_1, LED1_PIN, NEO_GRB, pixels1);
tinyNeoPixel leds2 = tinyNeoPixel(NUMPIXELS_2, LED2_PIN, NEO_GRB, pixels2);

ClickButton toggleButton(BUTTON_PIN, LOW, CLICKBTN_PULLUP);

bool on = true;
uint8_t mode = EEPROM.read(EEPROM_ADDRESS);

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  if (mode == 255) mode = 0;
}

uint8_t i;
uint16_t cycle;
unsigned long delayTime;

void loop() {
  updateButton();
  
  while(millis() > delayTime) {
    for (i = 0; i < NUMPIXELS_1; i++) {
      leds1.setPixelColor(i, getColor(i));
    }
  
    for (i = 0; i < NUMPIXELS_2; i++) {
      leds2.setPixelColor(i, getColor(i));
    }
  
    leds1.show();
    leds2.show();
  
    if (mode == 0) delayTime = millis() + random(50) + 100;
    if (mode == 1) delayTime = millis() + 1;
    
    cycle++;
  }
}

void updateButton() {
  toggleButton.Update();

  if (toggleButton.clicks == 1) {
    on = !on;
  }

  if (toggleButton.clicks == -1) {
    mode = (mode + 1) % 2;

    EEPROM.write(EEPROM_ADDRESS, mode);
  }
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b) {
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

uint32_t getColor(int index) {
  uint32_t color;
  if (mode == 0) color = Color(random(80) + 170, random(30) + 40, 0);
  if (mode == 1) color = ColorHSV(cycle, 255, 150);

  if (!on) color = Color(0,0,0);

  return color;
}

// Code from megaTinyCore: https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/libraries/tinyNeoPixel_Static/tinyNeoPixel_Static.cpp
uint32_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  uint8_t r, g, b;

  // Remap 0-65535 to 0-1529.
  hue = (hue * 1530L + 32768) / 65536;

  // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
  if (hue < 510) {         // Red to Green-1
    b = 0;
    if (hue < 255) {       //   Red to Yellow-1
      r = 255;
      g = hue;            //     g = 0 to 254
    } else {              //   Yellow to Green-1
      r = 510 - hue;      //     r = 255 to 1
      g = 255;
    }
  } else if (hue < 1020) { // Green to Blue-1
    r = 0;
    if (hue <  765) {      //   Green to Cyan-1
      g = 255;
      b = hue - 510;      //     b = 0 to 254
    } else {              //   Cyan to Blue-1
      g = 1020 - hue;     //     g = 255 to 1
      b = 255;
    }
  } else if (hue < 1530) { // Blue to Red-1
    g = 0;
    if (hue < 1275) {      //   Blue to Magenta-1
      r = hue - 1020;     //     r = 0 to 254
      b = 255;
    } else {              //   Magenta to Red-1
      r = 255;
      b = 1530 - hue;     //     b = 255 to 1
    }
  } else {                // Last 0.5 Red (quicker than % operator)
    r = 255;
    g = b = 0;
  }

  // Apply saturation and value to R,G,B, pack into 32-bit result:
  uint32_t v1 =   1 + val; // 1 to 256; allows >>8 instead of /255
  uint16_t s1 =   1 + sat; // 1 to 256; same reason
  uint8_t  s2 = 255 - sat; // 255 to 0
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
          (((((g * s1) >> 8) + s2) * v1) & 0xff00)       |
          (((((b * s1) >> 8) + s2) * v1)           >> 8);
}
