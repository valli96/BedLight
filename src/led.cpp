#include "led.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"

namespace led {
namespace {
int current = 0;
int lastOn = config::PWM_MAX;
}

void begin() {
  pinMode(config::LED_PIN, OUTPUT);
  analogWriteFreq(config::PWM_FREQ_HZ);
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

int lastOnLevel() { return lastOn; }

void restore(int level, int lastOnLevel) {
  lastOn = constrain(lastOnLevel, 1, config::PWM_MAX);
  set(level);
}

void step(int steps) {
  constexpr int pct = config::DIM_STEP_PERCENT;
  int value = current;
  for (; steps > 0; --steps) value += max(1, value * pct / 100);
  for (; steps < 0; ++steps) value -= max(1, value * pct / (100 + pct));
  set(value);
}

void setBrightness(int brightness) {
  if (brightness <= 0) {
    set(0);
    return;
  }
  const float ratio = min(brightness, config::BRIGHTNESS_MAX) / float(config::BRIGHTNESS_MAX);
  set(max(1, int(lroundf(config::PWM_MAX * ratio * ratio))));
}

int brightness() {
  if (current == 0) return 0;
  return max(1, int(lroundf(config::BRIGHTNESS_MAX * sqrtf(current / float(config::PWM_MAX)))));
}
}
