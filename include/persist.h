#pragma once

#include <stdint.h>

namespace persist {
// Kept in RTC memory: survives a software restart, not a power loss.
struct State {
  int32_t level = -1;  // -1: no light state to restore
  int32_t lastOn = 0;
  uint32_t watchdogRestarts = 0;
};

// Returns the stored state once; the light part is cleared so it is never restored twice.
State take();
void store(const State& state);
}
