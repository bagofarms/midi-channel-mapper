#include <MIDI.h>
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment matrix = Adafruit_7segment();

// Potentiometer inputs
#define MAX_POT_VALUE 1024
#define CHAN_IN_POT 2
#define CHAN_OUT_POT 3

// Button
#define BUTTON_PIN 2

#define NO_OUTPUT 0

#define DEBUG true

MIDI_CREATE_DEFAULT_INSTANCE();

// byte prog_pass_channel = 14; // Channel to pass through program change events
byte in_pot_channel = 0;
byte out_pot_channel = 0;

// Array to hold inputs and outputs.  Index is input channel, value is output channel.
// 0 = No output, 1 = Midi Channel 1, ..., 16 = Midi Channel 16
byte channel_map[17] = {NO_OUTPUT, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// Stuff that only runs every so often
#define LOOP_DELAY 500
int loop_ticks = 0;

int button_state;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  //attachInterrupt(0, pin_ISR, LOW);

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

  getPotInputs();

  // Display
  matrix.begin(0x70);
  matrix.setBrightness(1);
  matrix.drawColon(true);
  displayChannels();
}

void loop() {
  MIDI.read();
  
  button_state = digitalRead(BUTTON_PIN);
  if(button_state = LOW){
    updateChannels();
  }

  // Run the pot inputs and display channels every 100 loops
  if(++loop_ticks == LOOP_DELAY){
    displayChannels();
    getPotInputs();
    loop_ticks = 0;
  }
}

byte getOutChannel(byte in_channel) {
  return channel_map[in_channel];
}

// -------------------------------------------
// MIDI Handlers:
// -------------------------------------------

void handleProgramChange(byte channel, byte number) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendProgramChange(number, out_channel);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendNoteOn(note, velocity, out_channel);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendNoteOff(note, velocity, out_channel);
}

void handlePitchBend(byte channel, int bend) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendPitchBend(bend, out_channel);
}

void handleControlChange(byte channel, byte number, byte value) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendControlChange(number, value, out_channel);
}

void handleAfterTouchPolyPressure(byte channel, byte note, byte pressure) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendAfterTouch(note, pressure, out_channel);
}

void handleAfterTouchChannelPressure(byte channel, byte pressure) {
  byte out_channel = getOutChannel(channel);
  if(out_channel == NO_OUTPUT) {
    return;
  }

  MIDI.sendAfterTouch(pressure, out_channel);
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
  MIDI.sendRealTime(midi::Clock);
}

void handleStart() {
  MIDI.sendRealTime(midi::Start);
}

void handleContinue() {
  MIDI.sendRealTime(midi::Continue);
}

void handleStop() {
  MIDI.sendRealTime(midi::Stop);
}

void handleActiveSensing() {
  MIDI.sendRealTime(midi::ActiveSensing);
}

void handleSystemReset() {
  MIDI.sendRealTime(midi::SystemReset);
}

// -------------------------------------------
// Manual input methods:
// -------------------------------------------

void getPotInputs() {
  in_pot_channel = normalizePotInput(analogRead(CHAN_IN_POT));
  out_pot_channel = normalizePotInput(analogRead(CHAN_OUT_POT));
}

int normalizePotInput(float rawIn) {
  rawIn = rawIn/MAX_POT_VALUE;
  rawIn = rawIn * 16;
  return (int) (rawIn + 0.5f);
}

// -------------------------------------------
// Display methods:
// -------------------------------------------

void displayChannels(){
  displayLeft(in_pot_channel, false);
  displayRight(out_pot_channel, false);
}

void displayLeft(byte val, bool dot) {
  int leftDigit = val/10;
  if( leftDigit == 0 ) {
    matrix.writeDigitRaw(0, 0B000000000);
  } else {
    matrix.writeDigitNum(0, (val / 10), false);
  }
  matrix.writeDigitNum(1, val % 10, dot);
  matrix.writeDisplay();
}

void displayRight(int val, bool dot) {
  int leftDigit = val/10;
  if( leftDigit == 0 ) {
    matrix.writeDigitRaw(3, 0B000000000);
  } else {
    matrix.writeDigitNum(3, (val / 10), false);
  }
  matrix.writeDigitNum(4, val % 10, dot);
  matrix.writeDisplay();
}

void updateChannels() {
  channel_map[in_pot_channel] = out_pot_channel;
}

