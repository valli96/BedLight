#include "mqtt.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "led.h"
#include "secrets.h"

namespace mqtt {
namespace {
WiFiClient wifiClient;
PubSubClient client(wifiClient);
unsigned long lastAttempt = 0;

void onMessage(char* topic, byte* payload, unsigned int length) {
  char msg[16];
  if (strcmp(topic, config::TOPIC_POWER) != 0 || length >= sizeof(msg)) return;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  if (strcmp(msg, "TOGGLE") == 0) {
    led::toggle();
    return;
  }

  char* end = nullptr;
  const long value = strtol(msg, &end, 10);
  if (end != msg && *end == '\0' && value >= 0 && value <= config::PWM_MAX) {
    led::set(static_cast<int>(value));
  } else {
    Serial.printf("MQTT: ignoring payload '%s'\n", msg);
  }
}

void connect() {
  if (client.connect(config::MQTT_CLIENT_ID, secrets::MQTT_USER, secrets::MQTT_PASSWORD)) {
    Serial.println("MQTT: connected");
    client.subscribe(config::TOPIC_POWER);
  } else {
    Serial.printf("MQTT: connect failed, rc=%d\n", client.state());
  }
}
}

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
  client.setServer(secrets::MQTT_BROKER, secrets::MQTT_PORT);
  client.setCallback(onMessage);
}

void loop() {
  if (client.connected()) {
    client.loop();
    return;
  }
  if (WiFi.status() != WL_CONNECTED) return;

  const unsigned long now = millis();
  if (now - lastAttempt < config::MQTT_RECONNECT_MS) return;
  lastAttempt = now;
  connect();
}

bool publish(const char* topic, const char* payload) {
  return client.connected() && client.publish(topic, payload);
}
}
