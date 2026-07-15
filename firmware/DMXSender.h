/*
 * Minimal DMX512 sender for ESP32 with RS-485 transceiver (e.g. Grove DMX512 / SN75176).
 * No external DMX library. Uses 250k baud, 8N2; break + MAB then start code + 512 slots.
 *
 * From ESP-NOW-MIDI examples/client_dmx:
 * https://github.com/grantler-instruments/ESP-NOW-MIDI
 */
#ifndef DMX_SENDER_H
#define DMX_SENDER_H

#include <Arduino.h>

#if !defined(ESP32)
#error "DMXSender.h is for ESP32 only (Arduino-ESP32 core required)."
#endif

class DMXSender {
 public:
  DMXSender() = default;

  void begin(HardwareSerial& serial, uint8_t dePin, uint16_t numChannels = 512) {
    _serial = &serial;
    _dePin = dePin;
    _numChannels = (numChannels <= 512) ? numChannels : 512;
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);
    memset(_channels, 0, sizeof(_channels));
  }

  void writeByte(uint8_t value, uint16_t channel) {
    if (channel >= 1 && channel <= _numChannels) {
      _channels[channel - 1] = value;
    }
  }

  void update() {
    if (!_serial) return;
    digitalWrite(_dePin, HIGH);
    delayMicroseconds(2);

    _serial->updateBaudRate(83333);
    _serial->write((uint8_t)0x00);
    _serial->flush();
    _serial->updateBaudRate(250000);

    _serial->write((uint8_t)0x00);
    _serial->write(_channels, _numChannels);
    _serial->flush();

    digitalWrite(_dePin, LOW);
  }

 private:
  HardwareSerial* _serial = nullptr;
  uint8_t _dePin = 0;
  uint16_t _numChannels = 512;
  uint8_t _channels[512] = {};
};

#endif
