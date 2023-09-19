#include <tinyNeoPixel_Static.h>

#define LED_PIN PIN_PA6
#define SENSOR_PIN PIN_PA7

#define NUM_LEDS 30
#define ON_TIME 300 // (in seconds)
#define SPEED 100 // (time between animaions in millis)

byte pixels[NUM_LEDS * 3];
tinyNeoPixel leds = tinyNeoPixel(NUM_LEDS, LED_PIN, NEO_GRB, pixels);

uint8_t dir = 1;

uint32_t timer = millis() + 99999;
uint32_t anim = millis();

uint16_t hue1 = 45000;
uint16_t hue2 = 65000;

uint8_t offset = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  leds.begin();
}

void loop() {
  bool sensor = digitalRead(SENSOR_PIN);
  if (sensor) timer = millis() + (ON_TIME * 1000);

  if (millis() > anim) {
    anim = millis() + SPEED;
    offset++;
  }

  leds.setPixelColor(0, sensor ? 50 : 0, 0, 0);

  for (uint8_t i = 1; i < NUM_LEDS; i++) {
    uint32_t color = leds.ColorHSV(map((i + offset) % NUM_LEDS, 0, NUM_LEDS, hue1, hue2), 255, 30);
    leds.setPixelColor(i, millis() < timer ? color : 0);
  }

  leds.show();

  delay(10);
}
