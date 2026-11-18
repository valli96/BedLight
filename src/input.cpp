#include "input.h"

#include <OneButton.h>
#include <RotaryEncoder.h>

#include "config.h"
#include "led.h"
#include "mqtt.h"

namespace input {
namespace {
RotaryEncoder encoder(config::ENCODER_PIN_A, config::ENCODER_PIN_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(config::ENCODER_BUTTON_PIN, true);
long lastPosition = 0;

void IRAM_ATTR onEncoderChange() { encoder.tick(); }
}

void begin() {
  attachInterrupt(digitalPinToInterrupt(config::ENCODER_PIN_A), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(config::ENCODER_PIN_B), onEncoderChange, CHANGE);

  button.setPressMs(config::LONG_PRESS_MS);
  button.attachClick([] { led::toggle(); });
  button.attachDoubleClick([] { mqtt::publish(config::TOPIC_DOUBLE_CLICK, "1"); });
  button.attachLongPressStart([] { mqtt::publish(config::TOPIC_LONG_PRESS, "1"); });
}

void loop() {
  button.tick();

  const long position = encoder.getPosition();
  if (position == lastPosition) return;

  const float scale = config::SCALE_WITH_RPM ? 1.0f + encoder.getRPM() / config::RPM_SCALE_FACTOR : 1.0f;
  const int delta = static_cast<int>((position - lastPosition) * scale);
  lastPosition = position;
  led::set(led::level() + delta);
}
}
