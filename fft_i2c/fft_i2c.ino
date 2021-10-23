// Based off of: https://raw.githubusercontent.com/amcolash/ATTiny85/master/fft_i2c/fft_i2c.ino
#include <Wire.h>
#include <fix_fft.h>

//MUST BE POWER OF 2!
#define SAMPLES 64
#define HALF_SAMPLES SAMPLES/2
#define SCALAR 30

#define AUDIO_PIN PIN_PA7
#define LED_PIN PIN_PA6

uint8_t fft_power = (uint8_t) (log(SAMPLES) / log(2));
uint8_t i;

int16_t avg8;
uint16_t value, avg16, maxSample, minSample;

int8_t data[SAMPLES];
byte buff[HALF_SAMPLES];

bool status = false;

void requestEvent() {

  status = !status;
  digitalWrite(LED_PIN, status);

  // send 18 bytes (16 fft bars + avg)

  // Only send half of the data from FFT, average each two bands together
  for (i = 0; i < HALF_SAMPLES / 2; i++) {
    Wire.write(((buff[i * 2] + buff[i * 2 + 1]) / 2) & 0xFF);
  }

  avg16 /= SAMPLES;

  Wire.write(avg16 >> 8);
  Wire.write(avg16 & 255);

  Wire.write(minSample >> 8);
  Wire.write(minSample & 255);

  Wire.write(maxSample >> 8);
  Wire.write(maxSample & 255);
}

void setup() {
  for (i = 0; i < HALF_SAMPLES; i++) {
    buff[i] = 0;
  }

  Wire.begin(8);
  Wire.onRequest(requestEvent);

  pinMode(LED_PIN, OUTPUT);

  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_512CLK_gc); //enable the WDT, with a 0.512s timeout (WARNING: delay > 0.5 will reset)
}

void loop() {
  //  reset watchdog timer
  __asm__ __volatile__ ("wdr"::);

  avg8 = 0;
  avg16 = 0;

  minSample = 512;
  maxSample = 512;

  // Try to fix issues with first band
  for (i = 0; i < 10; i++) {
    analogRead(AUDIO_PIN);
  }

  for (i = 0; i < SAMPLES; i++) {
    value = analogRead(AUDIO_PIN);
    //    data[i] = ((analogRead(AUDIO_PIN) * SCALAR) >> 2) - 128;  // convert to 8-bit value
    data[i] = (value >> 2) - 128;  // convert to 8-bit value w/o SCALAR

    avg8 += data[i];
    avg16 += value;

    minSample = min(minSample, value);
    maxSample = max(maxSample, value);
  }

  avg8 = avg8 / SAMPLES;
  //  avg16 /= SAMPLES;
  for (i = 0; i < SAMPLES; i++) {
    data[i] -= avg8;
  }

  fix_fftr(data, fft_power, 0);

  // fill i2c buffer with fft data
  for (i = 0; i < HALF_SAMPLES; i++) {
    buff[i] = max(data[i], 1) & 0xFF;
  }

  // Some clever manipulation to try and clean out some noisy stuff.
  // Discard 1st band and then assign 1st and 2nd band to the 2 components of 2nd band
  buff[0] = max(data[2], 1) & 0xFF;
  buff[1] = max(data[2], 1) & 0xFF;

  buff[2] = max(data[3], 1) & 0xFF;
  buff[3] = max(data[3], 1) & 0xFF;
}
