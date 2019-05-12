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

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(0, pin_ISR, LOW);

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

  // OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();  // You must do this after every draw action
  delay(500);
  displayChannels();
}

void loop() {
  MIDI.read();

  // Run the pot inputs and display channels every 100 loops
  if(++loop_ticks == LOOP_DELAY){
    //displayChannels();
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
  display.clearDisplay();
  //display.setTextSize(2);
  display.setTextColor(WHITE);
  //display.setCursor(0,0);
  //display.print(OLED_HEADER);

  display.setTextSize(5);

  display.setCursor(0,SIZE5_2DIGIT_YOFFSET);
  oledPrintWithLeadingZero(in_pot_channel);

  display.setCursor(SIZE5_2DIGIT_XOFFSET,SIZE5_2DIGIT_YOFFSET);
  oledPrintWithLeadingZero(out_pot_channel);

  display.display();
}

void oledPrintWithLeadingZero(byte val) {
  if(val == NO_OUTPUT) {
    display.print("--");
    return;
  }

  int leftDigit = val/10;
  if( leftDigit == 0 ) {
    display.print(0);
  } else {
    display.print(val / 10);
  }

  display.print(val % 10);
}

void pin_ISR() {
  channel_map[in_pot_channel] = out_pot_channel;
}

