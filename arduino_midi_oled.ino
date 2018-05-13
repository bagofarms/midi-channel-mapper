#include <MIDI.h>

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#include <Adafruit_SSD1306.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

// OLED Display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define SIZE5_2DIGIT_XOFFSET 68
#define SIZE5_2DIGIT_YOFFSET 24
#define OLED_HEADER " MIDItron"

// Potentiometer inputs
#define MAX_POT_VALUE 1024
#define CHAN_IN_POT 2
#define CHAN_OUT_POT 3

#define DEBUG true

MIDI_CREATE_DEFAULT_INSTANCE();

// When output pot is set to 17, make it dynamic
byte dyn_channel = 1; // Programmatically selected
byte man_channel = 1; // Manually selected
byte in_channel = 1;  // Input channel
byte prog_pass_channel = 14; // Channel to pass through program change events

void setup() {
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleAfterTouchPoly(handleAfterTouchPolyPressure);
  MIDI.setHandleAfterTouchChannel(handleAfterTouchChannelPressure);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  MIDI.setHandleSongPosition(handleSongPosition);
  MIDI.setHandleSongSelect(handleSongSelect);
  MIDI.setHandleTuneRequest(handleTuneRequest);
  MIDI.setHandleClock(handleClock);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleActiveSensing(handleActiveSensing);
  MIDI.setHandleSystemReset(handleSystemReset);
  // (SysEx messages are not supported)

  MIDI.begin(MIDI_CHANNEL_OMNI);

  MIDI.turnThruOff();

  // OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();  // You must do this after every draw action
  delay(500);
  displayChannels();
}

void loop() {
  MIDI.read();
}

byte getOutChannel() {
  // TODO: Switch between the dynamic and manual override channel
  return dyn_channel;
}

// -------------------------------------------
// MIDI Handlers:
// -------------------------------------------

void handleProgramChange(byte channel, byte number) {
  // If we're on the input channel
  if(channel == in_channel) {
    dyn_channel = number;
    dyn_channel += 1; // Adding 1 because the midi library is 1-indexed
    displayChannels();
  } else if(channel == prog_pass_channel) {
    MIDI.sendProgramChange(number, prog_pass_channel);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if(channel == in_channel) {
    if(DEBUG) { displayNoteOLED(channel, note, velocity); }
    MIDI.sendNoteOn(note, velocity, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendNoteOn(note, velocity, prog_pass_channel);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if(channel == in_channel) {
    if(DEBUG) { displayNoteOLED(channel, note, velocity); }
    MIDI.sendNoteOff(note, velocity, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendNoteOff(note, velocity, prog_pass_channel);
  }
}

void handlePitchBend(byte channel, int bend) {
  if(channel == in_channel) {
    if(DEBUG) { displayPitchBend(bend); }
    MIDI.sendPitchBend(bend, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendPitchBend(bend, prog_pass_channel);
  }
}

void handleControlChange(byte channel, byte number, byte value) {
  if(channel == in_channel) {
    if(DEBUG) { displayControlChange(number, value); }
    MIDI.sendControlChange(number, value, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendControlChange(number, value, prog_pass_channel);
  }
}

void handleAfterTouchPolyPressure(byte channel, byte note, byte pressure) {
  if(channel == in_channel) {
    if(DEBUG) { displayAfterTouchPolyPressure(note, pressure); }
    MIDI.sendAfterTouch(note, pressure, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendAfterTouch(note, pressure, prog_pass_channel);
  }
}

void handleAfterTouchChannelPressure(byte channel, byte pressure) {
  if(channel == in_channel) {
    if(DEBUG) { displayAfterTouchChannelPressure(pressure); }
    MIDI.sendAfterTouch(pressure, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendAfterTouch(pressure, prog_pass_channel);
  }
}

void handleTimeCodeQuarterFrame(byte data) {
  MIDI.sendTimeCodeQuarterFrame(data);
}

void handleSongPosition(unsigned beats) {
  MIDI.sendSongPosition(beats);
}

void handleSongSelect(byte songnumber) {
  MIDI.sendSongSelect(songnumber);
}

void handleTuneRequest() {
  MIDI.sendTuneRequest();
}

// Real time messages (Messages that don't specify channels)
// (These messages don't have their own sendX method, instead we must use a
// lower-level send method from the library):
void handleClock() {
  MIDI.sendRealTime(MIDI::Clock);
}

void handleStart() {
  MIDI.sendRealTime(MIDI::Start);
}

void handleContinue() {
  MIDI.sendRealTime(MIDI::Continue);
}

void handleStop() {
  MIDI.sendRealTime(MIDI::Stop);
}

void handleActiveSensing() {
  MIDI.sendRealTime(MIDI::ActiveSensing);
}

void handleSystemReset() {
  MIDI.sendRealTime(MIDI::SystemReset);
}

// -------------------------------------------
// Display methods:
// -------------------------------------------

void displayChannels(){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(OLED_HEADER);

  display.setTextSize(5);

  display.setCursor(0,SIZE5_2DIGIT_YOFFSET);
  oledPrintWithLeadingZero(in_channel);

  display.setCursor(SIZE5_2DIGIT_XOFFSET,SIZE5_2DIGIT_YOFFSET);
  oledPrintWithLeadingZero(dyn_channel);

  display.display();
}

void displayNoteOLED(byte channel, byte note, byte velocity) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(channel);
  display.print(" -> ");
  display.print(dyn_channel);
  display.setCursor(0,16);
  display.println(note);
  display.println(velocity);
  display.display();
}

void displayPitchBend(int bend) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Pitch Bend");
  display.println(bend);
  display.display();
}

void displayControlChange(byte number, byte value) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("CC");
  display.println(number);
  display.println(value);
  display.display();
}

void displayAfterTouchChannelPressure(byte pressure) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("AftrTouchChan");
  display.println(pressure);
  display.display();
}

void displayAfterTouchPolyPressure(byte note, byte pressure) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("AftrTouchPoly");
  display.println(note);
  display.println(pressure);
  display.display();
}

void oledPrintWithLeadingZero(byte val) {
  int leftDigit = val/10;
  if( leftDigit == 0 ) {
    display.print(0);
  } else {
    display.print(val / 10);
  }

  display.print(val % 10);
}

