#pragma once
#include "enttec.h"

#define DEEMEX_VERSION_MAJOR 1
#define DEEMEX_VERSION_MINOR 0
#define DEEMEX_VERSION_PATCH 0

#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)
#define DEEMEX_TEENSY 1
#elif defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#define DEEMEX_ESP32 1
#else
#error "Unsupported platform: build for Teensy 4.x or ESP32"
#endif

#define UPDATE_DISPLAY_INTERVAL 64
#define MAX_HISTORY 5
#define SCREEN_ADDRESS 0x3C

#if DEEMEX_TEENSY
// Teensy 4.1: USB type Serial+MIDI
#define HAS_DISPLAY 1
#define BUTTON_PIN 2
#define SERIAL_BAUD 57600
#elif DEEMEX_ESP32
// Grove DMX512 / SN75176: TX on GPIO 21, DE on GPIO 4
#define HAS_DISPLAY 1
#define DMX_TX_PIN 21
#define DMX_DE_PIN 4
#define BUTTON_PIN 16
#define SERIAL_BAUD 57600
#endif
