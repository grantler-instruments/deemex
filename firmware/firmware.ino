//teensy 4.1
//usb type: serial+midi
// extern "C" {
//   // Declare the USB descriptor variables we want to override
//   extern char usb_string_manufacturer_name[];
//   extern uint16_t usb_string_manufacturer_name_len;
//   extern char usb_string_product_name[];
//   extern uint16_t usb_string_product_name_len;
// }


#include "config.h"
#include <AceButton.h>  //https://github.com/bxparks/AceButton
#include <MIDI.h>
#include <EEPROM.h>
#include <TeensyDMX.h>  //https://github.com/ssilverman/TeensyDMX
#include <Parameter.h>
#include "DmxMessageHistory.h"
#if HAS_DISPLAY == 1
#include "SSD1306Display.h"
static Display* display = nullptr;
static uint32_t lastDisplayUpdate = 0;
#endif

using namespace ace_button;

MIDI_CREATE_DEFAULT_INSTANCE();

namespace teensydmx = ::qindesign::teensydmx;
teensydmx::Sender dmxTx{ Serial5 };
AceButton button(static_cast<uint8_t>(BUTTON_PIN), 0);

// enttec pro
unsigned char state;
unsigned int dataSize;
unsigned int channel;

Parameter<bool> _midiModeActive;
Parameter<bool> _enttecModeActive;
Parameter<int> _noteOnStartChannel;


DmxMessageHistory messageHistory[MAX_HISTORY];
int messageIndex = 0;

// 14-bit CC handling
struct ChannelState {
  uint8_t msb = 0;
  uint8_t lsb = 0;
  bool msbReceived = false;
  bool lsbReceived = false;
  unsigned long lastUpdateTime = 0;
};

ChannelState channelStates[512];
const unsigned long PAIR_TIMEOUT = 20;  // ms - if both halves arrive within this window, combine them

void handleButtonEvent(AceButton* /*button*/, uint8_t eventType, uint8_t buttonState) {
  static unsigned long pressTime = 0;
  static unsigned long releaseTime = 0;

  unsigned long now = millis();

  Serial.print("Event: ");
  Serial.print(eventType);
  Serial.print(" State: ");
  Serial.print(buttonState);
  Serial.print(" Time: ");
  Serial.println(now);

  switch (eventType) {
    case AceButton::kEventPressed:
      pressTime = now;
      Serial.println("  -> Button PRESSED");
      break;

    case AceButton::kEventReleased:
      releaseTime = now;
      Serial.print("  -> Button RELEASED after ");
      Serial.print(releaseTime - pressTime);
      Serial.println(" ms");
      break;

    case AceButton::kEventLongPressed:
      _midiModeActive = !_midiModeActive;
      _enttecModeActive = !_enttecModeActive;

      EEPROM.write(EEPROM_MIDI_MODE_ADDR, _midiModeActive ? 1 : 0);
      EEPROM.write(EEPROM_ENTTEC_MODE_ADDR, _enttecModeActive ? 1 : 0);

      Serial.print("Modes saved - MIDI: ");
      Serial.print(_midiModeActive ? "ON" : "OFF");
      Serial.print(", Enttec: ");
      Serial.println(_enttecModeActive ? "ON" : "OFF");
      break;

    default:
      Serial.print("  -> Unknown event: ");
      Serial.println(eventType);
      break;
  }
}

void onNoteOn(byte channel, byte note, byte velocity) {
  // Use MIDI channels 1-4 to access all 512 DMX channels
  // Channel 1: DMX 1-127, Channel 2: DMX 128-254, Channel 3: DMX 255-381, Channel 4: DMX 382-508
  Serial.println("note on");

  if (channel < _noteOnStartChannel || channel > (_noteOnStartChannel + 4)) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }

  int dmxChannel = (channel - _noteOnStartChannel) * 127 + note;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxTx.set(dmxChannel, velocity * 2);
    addToHistory(dmxChannel, velocity * 2);
  }
}

void onNoteOff(byte channel, byte note, byte velocity) {
  // Use MIDI channels 1-4 to access all 512 DMX channels
  if (channel < _noteOnStartChannel || channel > (_noteOnStartChannel + 4)) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }

  int dmxChannel = (channel - _noteOnStartChannel) * 127 + note;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxTx.set(dmxChannel, 0);
    addToHistory(dmxChannel, 0);
  }
}

void onControlChange(byte channel, byte control, byte value) {
  // Determine if MSB (CC 0-31) or LSB (CC 32-63)
  bool isMSB = (control < 32);
  int baseControl = isMSB ? control : (control - 32);
  int dmxChannel = (channel - 1) * 32 + baseControl;

  if (dmxChannel >= 0 && dmxChannel < 512) {
    unsigned long now = millis();

    // Check if we should reset state due to timeout
    if (now - channelStates[dmxChannel].lastUpdateTime > PAIR_TIMEOUT) {
      channelStates[dmxChannel].msbReceived = false;
      channelStates[dmxChannel].lsbReceived = false;
    }

    if (isMSB) {
      channelStates[dmxChannel].msb = value;
      channelStates[dmxChannel].msbReceived = true;
      channelStates[dmxChannel].lastUpdateTime = now;
    } else {
      channelStates[dmxChannel].lsb = value;
      channelStates[dmxChannel].lsbReceived = true;
      channelStates[dmxChannel].lastUpdateTime = now;
    }

    // If we have both MSB and LSB, update DMX
    if (channelStates[dmxChannel].msbReceived && channelStates[dmxChannel].lsbReceived) {
      // Combine into 14-bit value
      uint16_t fullValue = (channelStates[dmxChannel].msb << 7) | channelStates[dmxChannel].lsb;

      // Scale to 8-bit DMX (0-16383 → 0-255)
      uint8_t dmxValue = fullValue >> 6;

      // Write to DMX (TeensyDMX channels are 1-indexed)
      dmxTx.set(dmxChannel + 1, dmxValue);
      addToHistory(dmxChannel + 1, dmxValue);

      // Clear flags for next pair
      channelStates[dmxChannel]
        .msbReceived = false;
      channelStates[dmxChannel].lsbReceived = false;
    }
  }
}

void onAfterTouchPoly(byte channel, byte note, byte velocity) {}
void onProgramChange(byte channel, byte program) {}
void onAfterTouch(byte channel, byte pressure) {}
void onPitchChange(byte channel, int pitch) {}
void onSystemExclusiveChunk(const byte* data, uint16_t length, bool last) {}
void onSystemExclusive(byte* data, unsigned int length) {}
void onTimeCodeQuarterFrame(byte data) {}
void onSongPosition(uint16_t beats) {}
void onSongSelect(byte songNumber) {}
void onTuneRequest() {}
void onClock() {}
void onStart() {}
void onContinue() {}
void onStop() {}
void onActiveSensing() {}
void onSystemReset() {}
void onRealTimeSystem(byte realtimebyte) {}

void readSerial() {
  unsigned char c;

  // enttec dmx pro
  while (Serial.available()) {
    c = Serial.read();

    if (c == DMX_PRO_START_MSG && state == DMX_PRO_END_MSG) {
      state = c;
    } else if (c == DMX_PRO_SEND_PACKET && state == DMX_PRO_START_MSG) {
      state = c;
    } else if (state == DMX_PRO_SEND_PACKET) {
      dataSize = c & 0xff;
      state = DMX_PRO_SEND_SIZE_LSB;
    } else if (state == DMX_PRO_SEND_SIZE_LSB) {
      dataSize += (c << 8) & 0xff00;
      state = DMX_PRO_SEND_SIZE_MSB;
    } else if (c == DMX_START_CODE && state == DMX_PRO_SEND_SIZE_MSB) {
      state = c;
      channel = 1;
    } else if (state == DMX_START_CODE && channel < dataSize) {
      dmxTx.set(channel, c);
      addToHistory(channel, c);
      channel++;
    } else if (state == DMX_START_CODE && channel == dataSize && c == DMX_PRO_END_MSG) {
      state = c;
    }
  }
}

void addToHistory(uint16_t channel, uint8_t value) {
  messageHistory[messageIndex].channel = channel;
  messageHistory[messageIndex].value = value;
  messageHistory[messageIndex].timestamp = millis();
  messageIndex = (messageIndex + 1) % MAX_HISTORY;
}

void setup() {
  Serial.begin(57600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Use internal pull-up
  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig->setLongPressDelay(1500);

  // Check if EEPROM has been initialized
  if (EEPROM.read(EEPROM_INIT_FLAG_ADDR) != EEPROM_INIT_VALUE) {
    // First time - write defaults
    EEPROM.write(EEPROM_MIDI_MODE_ADDR, 1);    // true
    EEPROM.write(EEPROM_ENTTEC_MODE_ADDR, 0);  // false
    EEPROM.write(EEPROM_NOTE_START_ADDR, 13);
    EEPROM.write(EEPROM_INIT_FLAG_ADDR, EEPROM_INIT_VALUE);
  }

  _midiModeActive.setup("midiMode", EEPROM.read(EEPROM_MIDI_MODE_ADDR));
  _enttecModeActive.setup("enttecMode", EEPROM.read(EEPROM_ENTTEC_MODE_ADDR));
  _noteOnStartChannel.setup("noteOnStartChannel", EEPROM.read(EEPROM_NOTE_START_ADDR));

  usbMIDI.begin();
  usbMIDI.setHandleNoteOn(onNoteOn);
  usbMIDI.setHandleNoteOff(onNoteOff);
  usbMIDI.setHandleAfterTouchPoly(onAfterTouchPoly);
  usbMIDI.setHandleControlChange(onControlChange);
  usbMIDI.setHandleProgramChange(onProgramChange);
  usbMIDI.setHandleTimeCodeQuarterFrame(onTimeCodeQuarterFrame);
  usbMIDI.setHandleSongSelect(onSongSelect);
  usbMIDI.setHandleTuneRequest(onTuneRequest);
  usbMIDI.setHandleClock(onClock);
  usbMIDI.setHandleStart(onStart);
  usbMIDI.setHandleContinue(onContinue);
  usbMIDI.setHandleStop(onStop);
  usbMIDI.setHandleActiveSensing(onActiveSensing);
  usbMIDI.setHandleSystemReset(onSystemReset);

  // Turn on the LED, for indicating activity
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);

  state = DMX_PRO_END_MSG;
  dmxTx.begin();

#if HAS_DISPLAY == 1
  static SSD1306Display ssd1306;

  display = &ssd1306;

  if (!display->begin()) {
    Serial.println("Display init failed");
    display = nullptr;
  }
#endif
}

void loop() {
  unsigned long now = millis();
  button.check();


  if (_midiModeActive) {
    usbMIDI.read();
  }
  if (_enttecModeActive) {
    readSerial();
  }
#if HAS_DISPLAY == 1
  if (display && (now - lastDisplayUpdate) >= UPDATE_DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    display->update(_enttecModeActive ? "enttec" : "midi", _noteOnStartChannel,
                    messageHistory,
                    MAX_HISTORY,
                    messageIndex);
  }
#endif
}