#pragma once

namespace mqtt {
void begin();
void loop();
bool publish(const char* topic, const char* payload);
}
