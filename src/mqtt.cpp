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
char deviceId[24];
unsigned long lastAttempt = 0;
unsigned long lastStatePublish = 0;
int publishedLevel = -1;

bool parseLevel(const char* msg, int& level) {
  char* end = nullptr;
  const long value = strtol(msg, &end, 10);
  if (end == msg || *end != '\0' || value < 0 || value > config::PWM_MAX) return false;
  level = static_cast<int>(value);
  return true;
}

void publishJson(const char* topic, const char* json) {
  client.publish(topic, reinterpret_cast<const uint8_t*>(json), strlen(json), true);
}

void publishTrigger(const char* name, const char* type, const char* topic) {
  char discoveryTopic[96];
  snprintf(discoveryTopic, sizeof(discoveryTopic), "%s/device_automation/%s/%s/config",
           config::HA_DISCOVERY_PREFIX, deviceId, name);

  char payload[256];
  snprintf(payload, sizeof(payload),
           R"({"automation_type":"trigger","type":"%s","subtype":"button_1","topic":"%s","payload":"1",)"
           R"("device":{"identifiers":["%s"]}})",
           type, topic, deviceId);
  publishJson(discoveryTopic, payload);
}

// Home Assistant MQTT discovery: retained config messages make the device appear automatically.
void publishDiscovery() {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/light/%s/config", config::HA_DISCOVERY_PREFIX, deviceId);

  char payload[768];
  snprintf(payload, sizeof(payload),
           R"({"name":null,"unique_id":"%s","object_id":"bedlight",)"
           R"("command_topic":"%s","state_topic":"%s",)"
           R"("brightness_command_topic":"%s","brightness_state_topic":"%s","brightness_scale":%d,)"
           R"("supported_color_modes":["brightness"],"availability_topic":"%s",)"
           R"("device":{"identifiers":["%s"],"name":"%s","model":"ESP-12E","manufacturer":"DIY"}})",
           deviceId, config::TOPIC_COMMAND, config::TOPIC_STATE, config::TOPIC_BRIGHTNESS_COMMAND,
           config::TOPIC_BRIGHTNESS_STATE, config::PWM_MAX, config::TOPIC_AVAILABILITY, deviceId,
           config::DEVICE_NAME);
  publishJson(topic, payload);

  publishTrigger("double_click", "button_double_press", config::TOPIC_DOUBLE_CLICK);
  publishTrigger("long_press", "button_long_press", config::TOPIC_LONG_PRESS);
  publishedLevel = -1;
}

void publishState() {
  const int level = led::level();
  client.publish(config::TOPIC_STATE, level > 0 ? "ON" : "OFF", true);
  if (level > 0) {
    char value[4];
    snprintf(value, sizeof(value), "%d", level);
    client.publish(config::TOPIC_BRIGHTNESS_STATE, value, true);
  }
  publishedLevel = level;
  lastStatePublish = millis();
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  char msg[16];
  if (length >= sizeof(msg)) return;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  int level = 0;
  if (strcmp(topic, config::TOPIC_COMMAND) == 0) {
    if (strcmp(msg, "ON") == 0) led::on();
    else if (strcmp(msg, "OFF") == 0) led::set(0);
  } else if (strcmp(topic, config::TOPIC_BRIGHTNESS_COMMAND) == 0) {
    if (parseLevel(msg, level)) led::set(level);
  } else if (strcmp(topic, config::HA_STATUS_TOPIC) == 0 && strcmp(msg, "online") == 0) {
    publishDiscovery();
  }
}

void connect() {
  if (!client.connect(deviceId, secrets::MQTT_USER, secrets::MQTT_PASSWORD,
                      config::TOPIC_AVAILABILITY, 0, true, "offline")) {
    Serial.printf("MQTT: connect failed, rc=%d\n", client.state());
    return;
  }
  Serial.println("MQTT: connected");
  client.subscribe(config::TOPIC_COMMAND);
  client.subscribe(config::TOPIC_BRIGHTNESS_COMMAND);
  client.subscribe(config::HA_STATUS_TOPIC);
  publishDiscovery();
  client.publish(config::TOPIC_AVAILABILITY, "online", true);
}
}

void begin() {
  snprintf(deviceId, sizeof(deviceId), "bedlight_%06x", ESP.getChipId());
  WiFi.mode(WIFI_STA);
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
  client.setBufferSize(config::MQTT_BUFFER_SIZE);
  client.setServer(secrets::MQTT_BROKER, secrets::MQTT_PORT);
  client.setCallback(onMessage);
}

void loop() {
  const unsigned long now = millis();

  if (client.connected()) {
    client.loop();
    if (led::level() != publishedLevel && now - lastStatePublish >= config::STATE_PUBLISH_MS) {
      publishState();
    }
    return;
  }
  if (WiFi.status() != WL_CONNECTED || now - lastAttempt < config::MQTT_RECONNECT_MS) return;
  lastAttempt = now;
  connect();
}

bool publish(const char* topic, const char* payload) {
  return client.connected() && client.publish(topic, payload);
}
}
