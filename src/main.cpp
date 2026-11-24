#include <Arduino.h>

#include "config.h"
#include "input.h"
#include "led.h"
#include "mqtt.h"
#include "persist.h"

void setup() {
  Serial.begin(config::SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
  const persist::State saved = persist::take();
  led::begin();
  if (saved.level >= 0) led::restore(saved.level, saved.lastOn);
  input::begin();
  mqtt::begin(saved.watchdogRestarts);
}

void loop() {
  input::loop();
  mqtt::loop();
  delay(10);
}
