#pragma once

#include <stdint.h>

namespace config {
constexpr uint32_t SERIAL_BAUD = 115200;

// GPIO3 is the UART RX pin, so serial runs TX-only.
constexpr uint8_t LED_PIN = 3;
constexpr uint8_t ENCODER_PIN_A = 12;
constexpr uint8_t ENCODER_PIN_B = 13;
constexpr uint8_t ENCODER_BUTTON_PIN = 14;

constexpr int PWM_MAX = 255;
constexpr bool SCALE_WITH_RPM = true;
constexpr float RPM_SCALE_FACTOR = 100.0f;
constexpr unsigned int LONG_PRESS_MS = 1000;

constexpr char DEVICE_NAME[] = "Bed Light";
constexpr unsigned long MQTT_RECONNECT_MS = 5000;
constexpr unsigned long STATE_PUBLISH_MS = 200;
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;

constexpr char TOPIC_AVAILABILITY[] = "bedroom/bedlight/status";
constexpr char TOPIC_STATE[] = "bedroom/bedlight/state";
constexpr char TOPIC_COMMAND[] = "bedroom/bedlight/set";
constexpr char TOPIC_BRIGHTNESS_STATE[] = "bedroom/bedlight/brightness";
constexpr char TOPIC_BRIGHTNESS_COMMAND[] = "bedroom/bedlight/brightness/set";
constexpr char TOPIC_DOUBLE_CLICK[] = "bedroom/bedlight/button/doubleclick";
constexpr char TOPIC_LONG_PRESS[] = "bedroom/bedlight/button/longpress";

constexpr char HA_DISCOVERY_PREFIX[] = "homeassistant";
constexpr char HA_STATUS_TOPIC[] = "homeassistant/status";
}
