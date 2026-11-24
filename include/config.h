#pragma once

#include <stdint.h>

namespace config {
constexpr uint32_t SERIAL_BAUD = 115200;

// D1 mini labels: LED = D1, encoder A = D6, B = D7, button = D5.
constexpr uint8_t LED_PIN = 5;
constexpr uint8_t ENCODER_PIN_A = 12;
constexpr uint8_t ENCODER_PIN_B = 13;
constexpr uint8_t ENCODER_BUTTON_PIN = 14;

constexpr int PWM_MAX = 1023;
constexpr uint32_t PWM_FREQ_HZ = 1000;
constexpr int BRIGHTNESS_MAX = 255;
constexpr int DIM_STEP_PERCENT = 10;
constexpr unsigned int LONG_PRESS_MS = 1000;

constexpr char DEVICE_NAME[] = "Bed Light";
constexpr char HOSTNAME[] = "bedlight";
constexpr unsigned long MQTT_RECONNECT_MS = 5000;
constexpr uint16_t MQTT_SOCKET_TIMEOUT_S = 3;
constexpr uint32_t TCP_CONNECT_TIMEOUT_MS = 3000;

// Self-healing: re-join Wi-Fi when it stays down, restart when MQTT stays unreachable.
// The restart delay doubles with every consecutive watchdog restart, up to the maximum.
constexpr unsigned long WIFI_RETRY_MS = 30000;
constexpr unsigned long OFFLINE_RESTART_MS = 5UL * 60 * 1000;
constexpr unsigned long OFFLINE_RESTART_MAX_MS = 60UL * 60 * 1000;
constexpr unsigned long STATE_PUBLISH_MS = 200;
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;

constexpr char TOPIC_AVAILABILITY[] = "bedroom/bedlight/status";
constexpr char TOPIC_STATE[] = "bedroom/bedlight/state";
constexpr char TOPIC_COMMAND[] = "bedroom/bedlight/set";
constexpr char TOPIC_BRIGHTNESS_STATE[] = "bedroom/bedlight/brightness";
constexpr char TOPIC_BRIGHTNESS_COMMAND[] = "bedroom/bedlight/brightness/set";
constexpr char TOPIC_DOUBLE_CLICK[] = "bedroom/bedlight/button/doubleclick";
constexpr char TOPIC_LONG_PRESS[] = "bedroom/bedlight/button/longpress";
constexpr char TOPIC_INFO[] = "bedroom/bedlight/info";

constexpr char HA_DISCOVERY_PREFIX[] = "homeassistant";
constexpr char HA_STATUS_TOPIC[] = "homeassistant/status";
}
