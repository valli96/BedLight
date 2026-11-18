#include <Arduino.h>

#include "config.h"
#include "input.h"
#include "led.h"
#include "mqtt.h"

void setup() {
  Serial.begin(config::SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
  led::begin();
  input::begin();
  mqtt::begin();
}

void loop() {
  input::loop();
  mqtt::loop();
  delay(10);
}
