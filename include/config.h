// Minimal configuration header
#pragma once

// Optional local overrides: create include/config.local.h (gitignored)
#if defined(__has_include)
#  if __has_include("config.local.h")
#    include "config.local.h"
#  endif
#endif

// MQTT defaults (can be overridden in config.local.h)
#ifndef MQTT_BROKER
static const char MQTT_BROKER[] = "192.168.0.1";
#endif
#ifndef MQTT_PORT
static const unsigned int MQTT_PORT = 1883u;
#endif
#ifndef MQTT_USER
static const char MQTT_USER[] = "mqtt_user";
#endif
#ifndef MQTT_PASSWORD
// Default MQTT password (override in config.local.h)
#endif

// Hardware defaults (pins)
#ifndef LED_PIN
static const int LED_PIN = 3;
#endif
#ifndef ENCODER_PIN_A
static const int ENCODER_PIN_A = 12;
#endif
#ifndef ENCODER_PIN_B
static const int ENCODER_PIN_B = 13;
#endif
#ifndef ENCODER_BUTTON_PIN
static const int ENCODER_BUTTON_PIN = 14;
#endif

// PWM range (0..ANALOG_MAX)
#ifndef ANALOG_MAX
static const int ANALOG_MAX = 255;
#endif

const bool SCALE_WITH_RPM  = true;
static const float RPM_SCALE_FACTOR = 100.0f;
