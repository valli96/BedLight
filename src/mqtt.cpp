#include "mqtt.h"
#include "config.h"
#include <PubSubClient.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static const char* CLIENT_ID = "bedlight_client";
static const char* SUB_TOPIC = "bedroom/bedlight/power";

// PWM state (0-255)
static int pwmValue = 0;
static int lastNonZeroPwm = 255; // remember last non-zero pwm for toggle


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to a null-terminated string (careful with size)
  constexpr size_t MAX_PAYLOAD = 128;
  char msg[MAX_PAYLOAD];
  size_t copyLen = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);
  memcpy(msg, payload, copyLen);
  msg[copyLen] = '\0';

  Serial.print("MQTT msg on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (strcmp(topic, SUB_TOPIC) != 0) return; // not our topic

  // If payload is the text "TOGGLE", toggle the LED
  if (strcmp(msg, "TOGGLE") == 0) {
    // toggle between 0 and lastNonZeroPwm using analogWrite
    if (pwmValue > 0) {
      analogWrite(LED_PIN, 0);
      pwmValue = 0;
    } else {
      int v = (lastNonZeroPwm > 0) ? lastNonZeroPwm : 255;
      analogWrite(LED_PIN, v);
      pwmValue = v;
    }
    Serial.print("Toggled PWM LED pin "); Serial.println(LED_PIN);
    return;
  }

  // Try parsing a numeric PWM value 0-255
  char* endptr = nullptr;
  long v = strtol(msg, &endptr, 10);
  if (endptr != msg && *endptr == '\0' && v >= 0 && v <= 255) {
    // It's a valid PWM value
  if (v > 0) lastNonZeroPwm = (int)v;
  pwmValue = (int)v;
  // analogWrite (0-255)
  analogWrite(LED_PIN, pwmValue);
  Serial.print("Set PWM "); Serial.print(LED_PIN); Serial.print(" = "); Serial.println(pwmValue);
  } else {
    // Unrecognized payload, ignore or log
    Serial.println("Unrecognized payload (not TOGGLE or 0-255)");
  }
}

// Try to connect to MQTT broker once; returns true on success.
static bool tryConnectMqtt() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.print("Attempting MQTT connect...");
  if (mqttClient.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("connected");
    mqttClient.subscribe(SUB_TOPIC);
    return true;
  }
  Serial.print("failed, rc=");
  Serial.println(mqttClient.state());
  return false;
}

void mqttInit() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize PWM for ESP8266
  // Initialize PWM (assume hardware supports analogWrite 0-255)
  analogWriteRange(255);
  pwmValue = 0;

  Serial.println("Starting WiFi (non-blocking)...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi begin called; continuing without blocking.");

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  // mqttLoop() will handle reconnect attempts periodically.
}

void mqttLoop() {
  // Service MQTT client regularly
  mqttClient.loop();

  static unsigned long lastReconnectAttempt = 0;
  constexpr unsigned long RECONNECT_INTERVAL_MS = 5000ul;

  // Only attempt reconnect if WiFi is connected
  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
      lastReconnectAttempt = now;
      tryConnectMqtt();
    }
  }
}

bool mqttPublish(const char* topic, const char* payload) {
  Serial.print("Publishing to ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(payload);
  if (!mqttClient.connected()) return false;
  return mqttClient.publish(topic, payload);
}

// LED helpers used by input module
void mqttToggleLed() {
  // toggle between 0 and lastNonZeroPwm using analogWrite
  if (pwmValue > 0) {
    analogWrite(LED_PIN, 0);
    pwmValue = 0;
  } else {
    int v = (lastNonZeroPwm > 0) ? lastNonZeroPwm : ANALOG_MAX;
    analogWrite(LED_PIN, v);
    pwmValue = v;
  }
}

void mqttSetLedPwm(int value) {
  if (value > 0) lastNonZeroPwm = value;
  pwmValue = value;
  analogWrite(LED_PIN, pwmValue);
}

int mqttGetLedPwm() {
  return pwmValue;
}
