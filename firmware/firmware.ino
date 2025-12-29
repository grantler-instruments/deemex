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
#include <MIDI.h>
#include <TeensyDMX.h>//https://github.com/ssilverman/TeensyDMX
#include <Parameter.h>
MIDI_CREATE_DEFAULT_INSTANCE();

namespace teensydmx = ::qindesign::teensydmx;
teensydmx::Sender dmxTx{ Serial5 };

// enttec pro
unsigned char state;
unsigned int dataSize;
unsigned int channel;

Parameter<bool> _midiModeActive;
Parameter<bool> _enttecModeActive;
Parameter<int> _noteOnStartChannel;

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

void onNoteOn(byte channel, byte note, byte velocity) {
  // Use MIDI channels 1-4 to access all 512 DMX channels
  // Channel 1: DMX 1-127, Channel 2: DMX 128-254, Channel 3: DMX 255-381, Channel 4: DMX 382-508
  if (channel < 1 || channel > 4) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }
  
  int dmxChannel = _noteOnStartChannel + (channel - 1) * 127 + note - 1;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxTx.set(dmxChannel, velocity * 2);
  }
}

void onNoteOff(byte channel, byte note, byte velocity) {
  // Use MIDI channels 1-4 to access all 512 DMX channels
  if (channel < 1 || channel > 4) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }
  
  int dmxChannel = _noteOnStartChannel + (channel - 1) * 127 + note - 1;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxTx.set(dmxChannel, 0);
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

      // Clear flags for next pair
      channelStates[dmxChannel].msbReceived = false;
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
      channel++;
    } else if (state == DMX_START_CODE && channel == dataSize && c == DMX_PRO_END_MSG) {
      state = c;
    }
  }
}

void setup() {
  Serial.begin(57600);
  _midiModeActive.setup("midiMode", true);
  _enttecModeActive.setup("enttecMode", false);
  _noteOnStartChannel.setup("noteOnStartChannel", 12);  // Default start at DMX channel 1

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
}

void loop() {
  if (_midiModeActive) {
    usbMIDI.read();
  }
  if (_enttecModeActive) {
    readSerial();
  }
}