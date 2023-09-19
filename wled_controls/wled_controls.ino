#include "ClickButton.h"

#define MAX_BRIGHTNESS 120
#define NUM_PRESETS 4
#define LEFT_BTN PIN_PA2
#define RIGHT_BTN PIN_PA3

int brightness = 70;
int brightnessDir = 1;
uint32_t brightnessTimer = 0;

#define LEFT 0
#define MIDDLE 1
#define RIGHT 2
bool segments[3] = {false, false, false};

ClickButton rightBtn(RIGHT_BTN, LOW, CLICKBTN_PULLUP);
int rightBtnFunction = 0;

ClickButton leftBtn(LEFT_BTN, LOW, CLICKBTN_PULLUP);
int leftBtnFunction = 0;

void setup() {
  // Wait a moment before starting (ESP RX pin must be high on boot)
  delay(250);

  Serial.begin(115200);
  Serial.println("\nSetting up LED controls\n");

  changeSegmentState(LEFT, true);
  changeSegmentState(MIDDLE, true);
  changeSegmentState(RIGHT, true);
  changePower(true);
}

void leftToggle() {
  bool l = segments[LEFT];
  bool m = segments[MIDDLE];

  if (l && m) {
    changeSegmentState(MIDDLE, false);
    changeSegmentState(RIGHT, false);
    changePower(true);
  } else if (l && !m) {
    changeSegmentState(LEFT, false);
    changePower(false);
  } else {
    changeSegmentState(LEFT, true);
    changeSegmentState(MIDDLE, true);
    changeSegmentState(RIGHT, true);
    changePower(true);
  }
}

void rightToggle() {
  bool r = segments[RIGHT];
  bool m = segments[MIDDLE];

  if (r && m) {
    changeSegmentState(LEFT, false);
    changeSegmentState(MIDDLE, false);
    changePower(true);
  } else if (r && !m) {
    changeSegmentState(RIGHT, false);
    changePower(false);
  } else {
    changeSegmentState(LEFT, true);
    changeSegmentState(MIDDLE, true);
    changeSegmentState(RIGHT, true);
    changePower(true);
  }
}

void changePower(bool state) {
  if (state) Serial.println("{'on': true}");
  else Serial.println("{'on': false}");
}

void changeSegmentState(uint8_t id, bool state) {
  segments[id] = state;

  Serial.print("{'seg':[{'id':");
  Serial.print(id);
  Serial.print(",'on':");
  Serial.print(state ? "true" : "false");
  Serial.println("}]}");
}

void handleBrightness() {
  if (millis() > brightnessTimer) {
    brightness = brightness + brightnessDir * 2;

    if (brightness > MAX_BRIGHTNESS) brightnessDir = -1;
    if (brightness < 1) brightnessDir = 1;

    Serial.print("{'bri':");
    Serial.print(constrain(brightness, 1, MAX_BRIGHTNESS));
    Serial.println("}");

    brightnessTimer = millis() + 16 + abs(MAX_BRIGHTNESS - brightness) / 3;
    if (brightness < 16) brightnessTimer += 50;
  }
}

void nextPreset() {
  Serial.print("{'ps':'1~");
  Serial.print(NUM_PRESETS);
  Serial.println("~'}");
}

void loop() {
  leftBtn.Update();
  if (leftBtn.clicks != 0) leftBtnFunction = leftBtn.clicks;

  rightBtn.Update();
  if (rightBtn.clicks != 0) rightBtnFunction = rightBtn.clicks;

  if (leftBtnFunction == 1) leftToggle();
  if (rightBtnFunction == 1) rightToggle();

  if (leftBtnFunction == 2 || rightBtnFunction == 2) nextPreset();

  if (leftBtnFunction < 0 && leftBtn.depressed) handleBrightness();
  else leftBtnFunction = 0;

  if (rightBtnFunction < 0 && rightBtn.depressed) handleBrightness();
  else rightBtnFunction = 0;
}