#pragma once

#include <stdint.h>

namespace mqtt {
void begin(uint32_t watchdogRestarts);
void loop();
bool publish(const char* topic, const char* payload);
}
