// Teensy 4.1: USB type Serial+MIDI
// ESP32: ESP-NOW MIDI client + DMX512 (install ESP-NOW-MIDI library)

#include "config.h"
#include <AceButton.h>
#include "DmxMessageHistory.h"
#include "SettingsStorage.h"
#include "platform.h"

#if DEEMEX_TEENSY
  #include <MIDI.h>
  MIDI_CREATE_DEFAULT_INSTANCE();
  namespace teensydmx = ::qindesign::teensydmx;
  teensydmx::Sender dmxTx{ Serial5 };
#elif DEEMEX_ESP32
  #include "enomik_client.h" //https://github.com/grantler-instruments/ESP-NOW-MIDI
  DMXSender dmx;
  HardwareSerial dmxSerial(1);
  enomik::Client espnowClient;
#endif

#if HAS_DISPLAY == 1
#include "SSD1306Display.h"
static Display* display = nullptr;
static uint32_t lastDisplayUpdate = 0;
#endif

using namespace ace_button;

AceButton button(static_cast<uint8_t>(BUTTON_PIN), 0);

unsigned char state;
unsigned int dataSize;
unsigned int channel;

bool _midiModeActive = true;
bool _enttecModeActive = false;
int _noteOnStartChannel = 13;

DmxMessageHistory messageHistory[MAX_HISTORY];
int messageIndex = 0;

struct ChannelState {
  uint8_t msb = 0;
  uint8_t lsb = 0;
  bool msbReceived = false;
  bool lsbReceived = false;
  unsigned long lastUpdateTime = 0;
};

ChannelState channelStates[512];
const unsigned long PAIR_TIMEOUT = 20;

void addToHistory(uint16_t channel, uint8_t value);

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

      saveModeSettings(_midiModeActive, _enttecModeActive);

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
  if (channel < _noteOnStartChannel || channel > (_noteOnStartChannel + 4)) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }

  int dmxChannel = (channel - _noteOnStartChannel) * 127 + note;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxWrite(dmxChannel, velocity * 2);
    addToHistory(dmxChannel, velocity * 2);
  }
}

void onNoteOff(byte channel, byte note, byte /*velocity*/) {
  if (channel < _noteOnStartChannel || channel > (_noteOnStartChannel + 4)) {
    return;
  }
  if (note < 1 || note > 127) {
    return;
  }

  int dmxChannel = (channel - _noteOnStartChannel) * 127 + note;
  if (dmxChannel >= 1 && dmxChannel <= 512) {
    dmxWrite(dmxChannel, 0);
    addToHistory(dmxChannel, 0);
  }
}

void onControlChange(byte channel, byte control, byte value) {
  bool isMSB = (control < 32);
  int baseControl = isMSB ? control : (control - 32);
  int dmxChannel = (channel - 1) * 32 + baseControl;

  if (dmxChannel >= 0 && dmxChannel < 512) {
    unsigned long now = millis();

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

    if (channelStates[dmxChannel].msbReceived && channelStates[dmxChannel].lsbReceived) {
      uint16_t fullValue = (channelStates[dmxChannel].msb << 7) | channelStates[dmxChannel].lsb;
      uint8_t dmxValue = fullValue >> 6;

      dmxWrite(dmxChannel + 1, dmxValue);
      addToHistory(dmxChannel + 1, dmxValue);

      channelStates[dmxChannel].msbReceived = false;
      channelStates[dmxChannel].lsbReceived = false;
    }
  }
}

void onAfterTouchPoly(byte /*channel*/, byte /*note*/, byte /*velocity*/) {}
void onProgramChange(byte /*channel*/, byte /*program*/) {}
void onAfterTouch(byte /*channel*/, byte /*pressure*/) {}
#if DEEMEX_TEENSY
void onPitchChange(byte /*channel*/, int /*pitch*/) {}
#else
void onPitchBend(byte /*channel*/, int /*value*/) {}
#endif
void onTimeCodeQuarterFrame(byte /*data*/) {}
void onSongPosition(uint16_t /*beats*/) {}
void onSongSelect(byte /*songNumber*/) {}
void onTuneRequest() {}
void onClock() {}
void onStart() {}
void onContinue() {}
void onStop() {}
void onActiveSensing() {}
void onSystemReset() {}

void registerMidiHandlers() {
#if DEEMEX_TEENSY
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
#elif DEEMEX_ESP32
  espnowClient.setHandleNoteOn(onNoteOn);
  espnowClient.setHandleNoteOff(onNoteOff);
  espnowClient.setHandleControlChange(onControlChange);
  espnowClient.setHandleProgramChange(onProgramChange);
  espnowClient.setHandlePitchBend(onPitchBend);
  espnowClient.setHandleAfterTouchChannel(onAfterTouch);
  espnowClient.setHandleAfterTouchPoly(onAfterTouchPoly);
#endif
}

void initMidi() {
#if DEEMEX_TEENSY
  usbMIDI.begin();
#elif DEEMEX_ESP32
  WiFi.mode(WIFI_STA);
  espnowClient.begin();
  // Register with dongle on first message
  espnowClient.sendControlChange(127, 127, 16);
#endif
  registerMidiHandlers();
}

void pollMidi() {
#if DEEMEX_TEENSY
  usbMIDI.read();
#elif DEEMEX_ESP32
  espnowClient.loop();
#endif
}

void readSerial() {
  unsigned char c;

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
      dmxWrite(channel, c);
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
  Serial.begin(SERIAL_BAUD);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  buttonConfig->setLongPressDelay(1500);

  const DeemexSettings settings = loadSettings();
  _midiModeActive = settings.midiModeActive;
  _enttecModeActive = settings.enttecModeActive;
  _noteOnStartChannel = settings.noteOnStartChannel;

  initMidi();

  pinMode(LED_BUILTIN, OUTPUT);
  setLedHigh();

  state = DMX_PRO_END_MSG;
  dmxBegin();

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
    pollMidi();
  }
  if (_enttecModeActive) {
    readSerial();
  }

  dmxPoll();

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
