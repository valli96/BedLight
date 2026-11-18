# BedLight

ESP8266 firmware that controls the LED strip behind my bed. You can dim the light with a rotary encoder on the device, or from Home Assistant, which discovers it automatically over MQTT.

## Hardware

- **ESP-12E (ESP8266)**: the microcontroller.
- **LED strip**: a plain 12 V single-color strip. A MOSFET switches it, and the ESP drives the MOSFET with PWM to set the brightness.
- **USB-C Power Delivery board**: a separate PCB that requests 12 V from a USB-C PD power supply and powers the whole device.
- **Voltage regulator**: converts the 12 V down to the 3.3 V the ESP needs.
- **Rotary encoder with push button**: the local control.

```
USB-C PD charger ──► PD board (12 V) ──┬──► MOSFET ──► LED strip
                                       │       ▲
                                       │       │ PWM (GPIO3)
                                       └──► Regulator (3.3 V) ──► ESP-12E ◄── Rotary encoder
```

### Pinout

| Function           | GPIO |
|--------------------|------|
| MOSFET gate (PWM)  | 3    |
| Encoder A          | 12   |
| Encoder B          | 13   |
| Encoder button     | 14   |

GPIO3 is also the UART RX pin, so the serial port only sends (TX) and is used for log output only.

## Controls

### Rotary encoder

| Action        | Effect                                                                      |
|---------------|-----------------------------------------------------------------------------|
| Turn          | Dims the light up or down (PWM 0–255).                                      |
| Single click  | Turns the light off, or back on at the last brightness it had.              |
| Double click  | Publishes `1` to `bedroom/bedlight/button/doubleclick`.                     |
| Long press (1 s) | Publishes `1` to `bedroom/bedlight/button/longpress`.                    |

The double click and long press don't change the light directly. They show up as device triggers in Home Assistant, so you can use them in automations, such as switching other lights in the room.

### Speed-dependent dimming

How much the brightness changes on each encoder step depends on how fast you turn the knob:

```
step = detents × (1 + RPM / 100)
```

- **Turning slowly** (close to 0 RPM) changes the brightness by about 1 per detent, so you can make fine adjustments.
- **Turning at 100 RPM** doubles the step, **at 300 RPM** it is 4 per detent, and so on.

This lets a quick flick go from off to full brightness, while a slow turn still gives fine control. You can tune it, or turn it off, with `SCALE_WITH_RPM` and `RPM_SCALE_FACTOR` in [include/config.h](include/config.h).

## Home Assistant

The light shows up in Home Assistant automatically through [MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery). You don't need any YAML. You only need the MQTT integration set up in Home Assistant.

1. When the ESP connects to the broker, it publishes retained config messages under `homeassistant/...`. These describe the device: a dimmable light plus two button triggers.
2. Home Assistant reads them and creates a **Bed Light** device. Its ID comes from the ESP chip, so it is the same after every reboot.
3. Controlling the light from Home Assistant sends a command to the ESP. The ESP reports its brightness back, including when you turn the knob, so the dashboard always matches the real light.
4. A last-will message marks the light as *unavailable* if the ESP goes offline. When Home Assistant restarts, it announces `online` on `homeassistant/status`, and the ESP sends its discovery messages again.

The double click and long press show up as **device triggers**, so you can pick them in the automation editor under *Device → Bed Light*.

### Topics

| Topic                                 | Direction | Payload                                  |
|---------------------------------------|-----------|------------------------------------------|
| `bedroom/bedlight/set`                | in        | `ON` / `OFF`                             |
| `bedroom/bedlight/brightness/set`     | in        | `0`–`255`                                |
| `bedroom/bedlight/state`              | out       | `ON` / `OFF` (retained)                  |
| `bedroom/bedlight/brightness`         | out       | `1`–`255` (retained)                     |
| `bedroom/bedlight/status`             | out       | `online` / `offline` (retained, LWT)     |
| `bedroom/bedlight/button/doubleclick` | out       | `1`                                      |
| `bedroom/bedlight/button/longpress`   | out       | `1`                                      |
| `homeassistant/status`                | in        | `online` triggers a new discovery        |

The device connects to Wi-Fi without blocking startup, so the encoder works even when there is no network. If the connection to the MQTT broker drops, it tries to reconnect every 5 seconds.

## Setup

1. Copy the secrets template and fill in your Wi-Fi and MQTT credentials. `include/secrets.h` is gitignored.
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
2. Build and flash with [PlatformIO](https://platformio.org):
   ```bash
   pio run -t upload
   ```
3. Optionally, watch the log output:
   ```bash
   pio device monitor
   ```

## Project layout

```
include/
  config.h               pins, topics, tuning values
  secrets.h.example      template for Wi-Fi/MQTT credentials
src/
  main.cpp               setup and main loop
  led.cpp                PWM brightness and on/off toggle
  input.cpp              rotary encoder and button handling
  mqtt.cpp               Wi-Fi, MQTT, Home Assistant discovery and state
```
