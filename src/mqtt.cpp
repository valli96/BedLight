#include "mqtt.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "led.h"
#include "persist.h"
#include "secrets.h"

namespace mqtt {
namespace {
WiFiClient wifiClient;
PubSubClient client(wifiClient);
char deviceId[24];
unsigned long lastAttempt = 0;
unsigned long lastStatePublish = 0;
int publishedLevel = -1;
wl_status_t lastWifiStatus = WL_IDLE_STATUS;
WiFiEventHandler disconnectHandler;
uint32_t wifiDisconnects = 0;
uint8_t lastDisconnectReason = 0;
uint32_t watchdogRestarts = 0;
unsigned long lastOnline = 0;
unsigned long lastWifiKick = 0;

bool parseLevel(const char* msg, int& level) {
  char* end = nullptr;
  const long value = strtol(msg, &end, 10);
  if (end == msg || *end != '\0' || value < 0 || value > config::BRIGHTNESS_MAX) return false;
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
           config::TOPIC_BRIGHTNESS_STATE, config::BRIGHTNESS_MAX, config::TOPIC_AVAILABILITY, deviceId,
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
    char value[12];
    snprintf(value, sizeof(value), "%d", led::brightness());
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
    if (parseLevel(msg, level)) led::setBrightness(level);
  } else if (strcmp(topic, config::HA_STATUS_TOPIC) == 0 && strcmp(msg, "online") == 0) {
    publishDiscovery();
  }
}

void publishInfo() {
  char payload[256];
  snprintf(payload, sizeof(payload),
           R"({"ip":"%s","rssi":%d,"reset_reason":"%s","uptime_s":%lu,)"
           R"("wifi_disconnects":%u,"last_disconnect_reason":%u,"watchdog_restarts":%u})",
           WiFi.localIP().toString().c_str(), WiFi.RSSI(), ESP.getResetReason().c_str(), millis() / 1000,
           wifiDisconnects, lastDisconnectReason, watchdogRestarts);
  publishJson(config::TOPIC_INFO, payload);
}

void logWifiChanges() {
  const wl_status_t status = WiFi.status();
  if (status == lastWifiStatus) return;
  lastWifiStatus = status;
  if (status == WL_CONNECTED) {
    Serial.printf("WiFi: connected, ip=%s rssi=%d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.printf("WiFi: status %d\n", status);
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
  publishInfo();

  if (watchdogRestarts > 0) {
    watchdogRestarts = 0;
    persist::store(persist::State{});
  }
}

void kickWifi(unsigned long now) {
  if (now - lastWifiKick < config::WIFI_RETRY_MS) return;
  lastWifiKick = now;
  Serial.println("WiFi: still down, forcing reconnect");
  WiFi.disconnect();
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
}

// Last resort after power glitches can leave the radio stuck: restart, keeping the light as it is.
void restartIfOfflineTooLong(unsigned long now) {
  const unsigned long limit =
      min(config::OFFLINE_RESTART_MS << min(watchdogRestarts, 4u), config::OFFLINE_RESTART_MAX_MS);
  if (now - lastOnline < limit) return;

  persist::State state;
  state.level = led::level();
  state.lastOn = led::lastOnLevel();
  state.watchdogRestarts = watchdogRestarts + 1;
  persist::store(state);
  Serial.printf("Watchdog: offline for %lu s, restarting\n", (now - lastOnline) / 1000);
  delay(20);
  ESP.restart();
}
}

void begin(uint32_t previousWatchdogRestarts) {
  watchdogRestarts = previousWatchdogRestarts;
  snprintf(deviceId, sizeof(deviceId), "bedlight_%06x", ESP.getChipId());
  Serial.printf("\nBoot: %s, reset reason: %s\n", deviceId, ESP.getResetReason().c_str());

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(config::HOSTNAME);
  WiFi.setAutoReconnect(true);
  disconnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    ++wifiDisconnects;
    lastDisconnectReason = event.reason;
  });
  WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
  lastOnline = lastWifiKick = millis();

  wifiClient.setTimeout(config::TCP_CONNECT_TIMEOUT_MS);
  client.setSocketTimeout(config::MQTT_SOCKET_TIMEOUT_S);
  client.setBufferSize(config::MQTT_BUFFER_SIZE);
  client.setServer(secrets::MQTT_BROKER, secrets::MQTT_PORT);
  client.setCallback(onMessage);
}

void loop() {
  const unsigned long now = millis();
  logWifiChanges();

  if (client.connected()) {
    lastOnline = lastWifiKick = now;
    client.loop();
    if (led::level() != publishedLevel && now - lastStatePublish >= config::STATE_PUBLISH_MS) {
      publishState();
    }
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    kickWifi(now);
  } else if (now - lastAttempt >= config::MQTT_RECONNECT_MS) {
    lastAttempt = now;
    lastWifiKick = now;
    connect();
  }
  restartIfOfflineTooLong(now);
}

bool publish(const char* topic, const char* payload) {
  return client.connected() && client.publish(topic, payload);
}
}
