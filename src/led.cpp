#include "led.h"

#include <Arduino.h>

#include "config.h"

namespace led {
namespace {
int current = 0;
int lastOn = config::PWM_MAX;
}

void begin() {
  pinMode(config::LED_PIN, OUTPUT);
  analogWriteRange(config::PWM_MAX);
  set(0);
}

void set(int value) {
  current = constrain(value, 0, config::PWM_MAX);
  if (current > 0) lastOn = current;
  analogWrite(config::LED_PIN, current);
}

int level() { return current; }

void on() {
  if (current == 0) set(lastOn);
}

void toggle() { set(current > 0 ? 0 : lastOn); }
}
