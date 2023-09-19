#include <ClickButton.h>
#include <EEPROM.h>
#include <PinButton.h>
#include <tinyNeoPixel_Static.h>

#define EEPROM_BRIGHTNESS 0
#define EEPROM_RAINBOW EEPROM_BRIGHTNESS + 1
#define EEPROM_COLOR EEPROM_RAINBOW + 1

#define LED_PIN PIN_PA7
#define BUTTON_PIN PIN_PA1
#define LDR_PIN PIN_PA6

#define NUM_PIXELS 5
#define SPEED 4
#define MAX_BRIGHTNESS 170
#define COLOR_STEPS 10

bool on = true;
bool rainbow = EEPROM.read(EEPROM_RAINBOW) == 0 ? false : true;
bool autoBrightness = EEPROM.read(EEPROM_BRIGHTNESS) == 0 ? false : true;

uint8_t targetBrightness = MAX_BRIGHTNESS;
uint8_t currentBrightness = targetBrightness;

byte pixels[NUM_PIXELS * 3];
tinyNeoPixel leds = tinyNeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB, pixels);

ClickButton toggleButton(BUTTON_PIN, LOW, CLICKBTN_PULLUP);
int buttonFn = 0;

long updateTimer = millis();
long ldrTimer = millis();
long colorTimer = millis();

/** Colors */
uint32_t black = leds.Color(0, 0, 0);
uint32_t red = leds.Color(150, 0, 0);
uint32_t green = leds.Color(0, 150, 0);
uint32_t blue = leds.Color(0, 0, 150);

uint16_t currentColor = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // Load current color from EEPROM (can't be run before setup)
  EEPROM.get(EEPROM_COLOR, currentColor);
}

void toggle() {
  on = !on;
}

void updateButton() {
  toggleButton.Update();

  if (toggleButton.clicks != 0) buttonFn = toggleButton.clicks;

  if (buttonFn == 1) toggle();
  if (buttonFn == 2) { toggleRainbow(false); nextColor(); }
  if (buttonFn == 3) toggleRainbow(!rainbow);
  if (buttonFn == 4) toggleAutoBrightness();
  
  if (buttonFn < 0 && toggleButton.depressed) toggleAutoBrightness(); // held down
  else buttonFn = 0;
}

void toggleAutoBrightness() {
  autoBrightness = !autoBrightness;
  EEPROM.update(EEPROM_BRIGHTNESS, autoBrightness);

  if (autoBrightness) flash(green);
  else flash(red);
}

void updateBrightness() {
  if (millis() > updateTimer) {
    if (on) {
      if (autoBrightness) {
        int brightness = (90 * targetBrightness / 100) + (10 * (analogRead(LDR_PIN) / 4) / 100); // no floating point math! (weights of 0.9 and 0.1)
        brightness = constrain(brightness, 5, MAX_BRIGHTNESS);
        targetBrightness = brightness;

        ldrTimer = millis() + 25;
      } else {
        targetBrightness = MAX_BRIGHTNESS;
      }


      if (currentBrightness < targetBrightness) currentBrightness += SPEED;
      if (currentBrightness > targetBrightness) currentBrightness -= SPEED;
    } else {
      if (currentBrightness > 0) currentBrightness = max(0, currentBrightness - SPEED);  
    }
    
    leds.setBrightness(currentBrightness);

    updateTimer = millis() + 25;
  }
}

void nextColor() {
  if (rainbow) currentColor += 3;
  else {
    currentColor += 4096;
    EEPROM.put(EEPROM_COLOR, currentColor);
  }
}

void toggleRainbow(bool value) {
  if (rainbow != value) {
    flash(blue);
  }

  if (rainbow && !value) currentColor = 0;

  rainbow = value;

  EEPROM.update(EEPROM_RAINBOW, rainbow);
}

void updateColor() {
  if (on && rainbow && millis() > colorTimer) {
    nextColor();
    colorTimer = millis() + 1;
  }

  // Fill with slightly offset hues
  for (int i = 0; i < NUM_PIXELS; i++) {
    leds.setPixelColor(i, leds.ColorHSV(currentColor + (i - 2) * 3000, 180, 255));
  }
  leds.show();
}

void loop() {
  updateButton();
  updateBrightness();
  updateColor();
}

/** Utils */

void fill(uint32_t color) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    leds.setPixelColor(i, color);
  }
  leds.show();
}


void flash(uint32_t color) {
  fill(black);
  delay(500);

  for (int i = 0; i < 3; i++) {
    fill(color);
    delay(500);

    fill(black);
    delay(400);
  }

  fill(black);
  delay(700);
}

// Read/write from https://forum.arduino.cc/t/how-i-can-write-long-num-in-eeprom-and-read/292990/2
void writeEepromUint16(uint16_t value, int location){
  EEPROM.write(location, value);
  EEPROM.write(location + 1, value >> 8);
}

int readEepromInt(int location){
  int val;

  val = (EEPROM.read(location + 1) << 8);
  val |= EEPROM.read(location);

  return val;
}