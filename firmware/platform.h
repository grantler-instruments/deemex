#pragma once

#include "config.h"

#if DEEMEX_TEENSY
#include <TeensyDMX.h>
namespace teensydmx = ::qindesign::teensydmx;
extern teensydmx::Sender dmxTx;
#elif DEEMEX_ESP32
#include "DMXSender.h"
extern DMXSender dmx;
extern HardwareSerial dmxSerial;
#endif

static inline void dmxWrite(uint16_t channel, uint8_t value) {
#if DEEMEX_TEENSY
  dmxTx.set(channel, value);
#elif DEEMEX_ESP32
  dmx.writeByte(value, channel);
#endif
}

static inline void dmxBegin() {
#if DEEMEX_TEENSY
  dmxTx.begin();
#elif DEEMEX_ESP32
  dmxSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);
  dmx.begin(dmxSerial, DMX_DE_PIN, 512);
#endif
}

static inline void dmxPoll() {
#if DEEMEX_ESP32
  dmx.update();
#endif
}

static inline void setLedHigh() {
#if DEEMEX_TEENSY
  digitalWriteFast(LED_BUILTIN, HIGH);
#else
  digitalWrite(LED_BUILTIN, HIGH);
#endif
}
