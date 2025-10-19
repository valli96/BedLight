#include <Arduino.h>
#include "config.h"
#include "mqtt.h"
#include "input.h"



void setup() {
  Serial.begin(115200);
  inputInit();
  mqttInit();
}

void loop() {
  inputLoop();
  mqttLoop();
  // Serial.println("Looping...");
  delay(10);
}