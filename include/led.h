#pragma once

namespace led {
void begin();
void set(int level);
int level();
void on();
void toggle();
int lastOnLevel();
void restore(int level, int lastOnLevel);

// Perceptual dimming: each step changes the level by a percentage, so the low end is very fine.
void step(int steps);

// Home Assistant brightness (0..BRIGHTNESS_MAX), gamma-mapped onto the PWM range.
void setBrightness(int brightness);
int brightness();
}
