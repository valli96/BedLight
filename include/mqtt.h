// mqtt.h - simple wrapper for PubSubClient based MQTT usage
#pragma once

#include <Arduino.h>

// Initialize WiFi and MQTT client. Call once from setup().
void mqttInit();

// Call frequently from loop() to keep connection alive and process messages.
void mqttLoop();

// Publish a message (returns true on success)
bool mqttPublish(const char* topic, const char* payload);

// Local LED control helpers (used by input module)
void mqttToggleLed();
void mqttSetLedPwm(int value); // 0-255
int mqttGetLedPwm();
