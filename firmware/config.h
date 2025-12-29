#pragma once
#include "enttec.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define HAS_DISPLAY 1
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; or run an i2c scanner
#define UPDATE_DISPLAY_INTERVAL 64
#define MAX_HISTORY 5

#define BUTTON_PIN 2
#define LONG_PRESS_TIME 1000
