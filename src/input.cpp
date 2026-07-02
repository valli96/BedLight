#include "input.h"
#include "config.h"
#include "mqtt.h"

#include <RotaryEncoder.h>
#include <OneButton.h>

RotaryEncoder *encoder = new RotaryEncoder(ENCODER_PIN_A, ENCODER_PIN_B, RotaryEncoder::LatchMode::TWO03);
OneButton *button = new OneButton(ENCODER_BUTTON_PIN, true); // true = active low

// Encoder position tracking
volatile int16_t encoderPos = 0;

// Called when button is clicked -> toggle LED
void onButtonClick()
{
  mqttToggleLed();
  // digitalWrite(3, !digitalRead(3));
}
// ISR attribute macro for ESP8266/ESP32
#if defined(ARDUINO_ARCH_ESP8266)
#define INPUT_ISR_ATTR ICACHE_RAM_ATTR
#else
#define INPUT_ISR_ATTR IRAM_ATTR
#endif

// ISR must be a plain function in IRAM and do minimal work. Use
// the library's service() method which is ISR-safe.
void INPUT_ISR_ATTR encoderISR()
{
  if (encoder)
    encoder->tick();
}
void INPUT_ISR_ATTR buttonISR()
{
  if (button)
    button->tick();
}

void inputInit()
{
  pinMode(3, OUTPUT);
  // Prepare pins

  // Attach interrupts to encoder pins (ISR will call encoder->service())
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON_PIN), buttonISR, CHANGE);

  // Setup button handlers; OneButton must be polled from loop() via tick().
  button->attachClick(onButtonClick);
  button->attachDoubleClick([](){
  mqttPublish("bedroom/bedlight/button/doubleclick", "1");
  });
  button->setPressMs(1000);
  button->attachLongPressStart([](){
    mqttPublish("bedroom/bedlight/button/longpress", "1");
  });
}

void inputLoop()
{
  button->tick();
  encoder->tick();

  int16_t pos = encoder->getPosition();
  float rpm = encoder->getRPM();
  // Serial.print("RPM: "); Serial.println(rpm);

  if (pos != encoderPos)
  {
    Serial.print("RPM: ");
    Serial.println(rpm);
    int delta = pos - encoderPos;
    encoderPos = pos;
    // Adjust PWM by delta (one step per detent)
    int curPwm = mqttGetLedPwm();
    Serial.println("Current PWM: " + String(curPwm));
    if (SCALE_WITH_RPM)
    {
      // Scale delta by RPM (e.g. 1 step per 10 RPM)
      float scale = 1 + rpm / RPM_SCALE_FACTOR;
      delta = static_cast<int>(delta * scale);
      Serial.println("Scaled delta: " + String(delta));
    } 
    int newPwm = curPwm + delta;
    if (newPwm < 0)
      newPwm = 0;
    if (newPwm > ANALOG_MAX)
      newPwm = ANALOG_MAX;
    mqttSetLedPwm(newPwm);
  }
}