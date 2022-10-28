#include <avr/sleep.h>

#define POWER PIN_PA3
#define SHUTDOWN PIN_PA7
#define TOGGLE_ON PIN_PA2
#define TOGGLE_OFF PIN_PA6
#define ESP_OUT PIN_PA1

#define WATCHDOG_SECONDS 15
#define WATCHDOG WATCHDOG_SECONDS * 1000
long sleepTime = -1;

void setup() {
  pinMode(POWER, OUTPUT);
  pinMode(SHUTDOWN, INPUT);
  pinMode(TOGGLE_ON, INPUT_PULLUP);
  pinMode(TOGGLE_OFF, INPUT_PULLUP);
  pinMode(ESP_OUT, OUTPUT);

  // Save power
  pinMode(PIN_PA0, OUTPUT);

  attachInterrupt(SHUTDOWN, shutdown, RISING);
  attachInterrupt(TOGGLE_ON, turnOn, FALLING);
  attachInterrupt(TOGGLE_OFF, turnOff, FALLING);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  digitalWrite(POWER, LOW);
  digitalWrite(ESP_OUT, LOW);

  // Go to sleep until a pin wakes things up
  sleep_cpu();
}

void shutdown() {
  digitalWrite(ESP_OUT, LOW);
  digitalWrite(POWER, LOW);

  // Go back to sleep until another interrupt
  sleepTime = -1;
  sleep_cpu();
}

void turnOn() {
  digitalWrite(ESP_OUT, HIGH);
  digitalWrite(POWER, HIGH);

  sleepTime = millis() + WATCHDOG;
}

void turnOff() {
  digitalWrite(ESP_OUT, LOW);
  digitalWrite(POWER, HIGH);

  sleepTime = millis() + WATCHDOG;
}

void loop() {
  if (sleepTime > -1 && millis() > sleepTime) shutdown();

  if (!digitalRead(TOGGLE_ON)) turnOn();
  if (!digitalRead(TOGGLE_OFF)) turnOff();

//  if (digitalRead(SHUTDOWN)) shutdown();
}
