#pragma once

#include "config.h"

struct DeemexSettings {
  bool midiModeActive = true;
  bool enttecModeActive = false;
  int noteOnStartChannel = 13;
};

#if DEEMEX_ESP32
#include <Preferences.h>

static constexpr const char* DEEMEX_PREFS_NAMESPACE = "deemex";

inline void saveModeSettings(bool midiModeActive, bool enttecModeActive) {
  Preferences prefs;
  prefs.begin(DEEMEX_PREFS_NAMESPACE, false);
  prefs.putBool("midi_mode", midiModeActive);
  prefs.putBool("enttec_mode", enttecModeActive);
  prefs.end();
}

inline void saveNoteOnStartChannel(int noteOnStartChannel) {
  Preferences prefs;
  prefs.begin(DEEMEX_PREFS_NAMESPACE, false);
  prefs.putUInt("note_start", noteOnStartChannel);
  prefs.end();
}

inline DeemexSettings loadSettings() {
  DeemexSettings settings;
  Preferences prefs;

  prefs.begin(DEEMEX_PREFS_NAMESPACE, true);
  if (!prefs.getBool("initialized", false)) {
    prefs.end();
    prefs.begin(DEEMEX_PREFS_NAMESPACE, false);
    prefs.putBool("initialized", true);
    prefs.putBool("midi_mode", settings.midiModeActive);
    prefs.putBool("enttec_mode", settings.enttecModeActive);
    prefs.putUInt("note_start", settings.noteOnStartChannel);
    prefs.end();
    return settings;
  }

  settings.midiModeActive = prefs.getBool("midi_mode", true);
  settings.enttecModeActive = prefs.getBool("enttec_mode", false);
  settings.noteOnStartChannel = prefs.getInt("note_start", 13);
  prefs.end();

  if (settings.noteOnStartChannel < 1 || settings.noteOnStartChannel > 16) {
    settings.noteOnStartChannel = 13;
    saveNoteOnStartChannel(settings.noteOnStartChannel);
  }

  return settings;
}

#else
#include <EEPROM.h>

#define EEPROM_MIDI_MODE_ADDR 0
#define EEPROM_ENTTEC_MODE_ADDR 1
#define EEPROM_NOTE_START_ADDR 2
#define EEPROM_INIT_FLAG_ADDR 10
#define EEPROM_INIT_VALUE 0xAB

inline void saveModeSettings(bool midiModeActive, bool enttecModeActive) {
  EEPROM.write(EEPROM_MIDI_MODE_ADDR, midiModeActive ? 1 : 0);
  EEPROM.write(EEPROM_ENTTEC_MODE_ADDR, enttecModeActive ? 1 : 0);
}

inline void saveNoteOnStartChannel(int noteOnStartChannel) {
  EEPROM.write(EEPROM_NOTE_START_ADDR, noteOnStartChannel);
}

inline DeemexSettings loadSettings() {
  DeemexSettings settings;

  if (EEPROM.read(EEPROM_INIT_FLAG_ADDR) != EEPROM_INIT_VALUE) {
    EEPROM.write(EEPROM_MIDI_MODE_ADDR, 1);
    EEPROM.write(EEPROM_ENTTEC_MODE_ADDR, 0);
    EEPROM.write(EEPROM_NOTE_START_ADDR, 13);
    EEPROM.write(EEPROM_INIT_FLAG_ADDR, EEPROM_INIT_VALUE);
  }

  settings.midiModeActive = EEPROM.read(EEPROM_MIDI_MODE_ADDR) != 0;
  settings.enttecModeActive = EEPROM.read(EEPROM_ENTTEC_MODE_ADDR) != 0;
  settings.noteOnStartChannel = EEPROM.read(EEPROM_NOTE_START_ADDR);

  if (settings.noteOnStartChannel < 1 || settings.noteOnStartChannel > 16) {
    settings.noteOnStartChannel = 13;
    saveNoteOnStartChannel(settings.noteOnStartChannel);
  }

  return settings;
}

#endif
