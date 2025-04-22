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
#include <TeensyDMX.h>
#include <Parameter.h>
MIDI_CREATE_DEFAULT_INSTANCE();


namespace teensydmx = ::qindesign::teensydmx;
teensydmx::Sender dmxTx{ Serial8 };

// enttec pro
unsigned char state;
unsigned int dataSize;
unsigned int channel;

Parameter<bool> _midiModeActive;
Parameter<bool> _enttecModeActive;


void onNoteOn(byte channel, byte note, byte velocity) {
  Serial.print("Note On, ch=");
  Serial.print(channel);
  Serial.print(", note=");
  Serial.print(note);
  Serial.print(", velocity=");
  Serial.print(velocity);
  Serial.println();

  dmxTx.set(note, velocity * 2);
}

void onNoteOff(byte channel, byte note, byte velocity) {
  // Serial.print("Note Off, ch=");
  // Serial.print(channel);
  // Serial.print(", note=");
  // Serial.print(note);
  // //Serial.print(", velocity=");
  // //Serial.print(velocity);
  // Serial.println();
  // dmxTx.set(note, 0);
}

void onControlChange(byte channel, byte control, byte value) {
  // Serial.print("Control Change, ch=");
  // Serial.print(channel);
  // Serial.print(", control=");
  // Serial.print(control);
  // Serial.print(", value=");
  // Serial.print(value);
  // Serial.println();
  // dmxTx.set(control, value*2);
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


  usbMIDI.begin();
  usbMIDI.setHandleNoteOn(onNoteOn);
  usbMIDI.setHandleNoteOff(onNoteOff);
  usbMIDI.setHandleAfterTouchPoly(onAfterTouchPoly);
  usbMIDI.setHandleControlChange(onControlChange);
  usbMIDI.setHandleProgramChange(onProgramChange);
  //  usbMIDI.setHandleAfterTouch(onAfterTouch);
  //  usbMIDI.setHandlePitchChange(onPitchChange);
  //  usbMIDI.setHandleSystemExclusive(onSystemExclusiveChunk);
  usbMIDI.setHandleTimeCodeQuarterFrame(onTimeCodeQuarterFrame);
  //  usbMIDI.setHandleSongPosition(onSongPosition);
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
