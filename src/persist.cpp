#include "persist.h"

#include <Arduino.h>

namespace persist {
namespace {
constexpr uint32_t MAGIC = 0xBED1A001;
constexpr uint32_t RTC_BLOCK = 32;  // the first 128 bytes of RTC user memory are reserved for OTA

struct Record {
  uint32_t magic;
  State state;
  uint32_t check;
};

uint32_t checksum(const State& s) {
  return MAGIC ^ (uint32_t(s.level) * 31u) ^ (uint32_t(s.lastOn) * 131u) ^ (s.watchdogRestarts * 1031u);
}
}

State take() {
  Record record;
  if (!ESP.rtcUserMemoryRead(RTC_BLOCK, reinterpret_cast<uint32_t*>(&record), sizeof(record)) ||
      record.magic != MAGIC || record.check != checksum(record.state)) {
    return State{};
  }
  State cleared = record.state;
  cleared.level = -1;
  store(cleared);
  return record.state;
}

void store(const State& state) {
  Record record{MAGIC, state, checksum(state)};
  ESP.rtcUserMemoryWrite(RTC_BLOCK, reinterpret_cast<uint32_t*>(&record), sizeof(record));
}
}
