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
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();

  // OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();  // You must do this after every draw action
  delay(500);
  displayChannels();
}

void loop() {
  // Write values to screen
  //writeOLED(chanIn, chanOut);
  MIDI.read();
}

void handleProgramChange(byte channel, byte number) {
  // If we're on the input channel
  if(channel == in_channel) {
    // Split off the first byte to find output channel to change to
    dyn_channel = number;
    dyn_channel += 1; // Adding 1 because the midi library is 1-indexed
    
    // Remove the first byte to send out
    // byte new_num = number & 0x0F;
    displayChannels();
  } else if(channel == prog_pass_channel) {
    MIDI.sendProgramChange(number, prog_pass_channel);
  }
}

void handleAftertouch(byte command, byte data1) {
  // Don't do anything here because aftertouch sends data too fast
  // and it will bog down the serial reads.
  Serial.write(command);
  Serial.write(data1);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if(channel == in_channel) {
    if(DEBUG){ displayNoteOLED(channel, note, velocity); }
    MIDI.sendNoteOn(note, velocity, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendNoteOn(note, velocity, prog_pass_channel);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if(channel == in_channel) {
    if(DEBUG){ displayNoteOLED(channel, note, velocity); }
    MIDI.sendNoteOff(note, velocity, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendNoteOff(note, velocity, prog_pass_channel);
  }
}

void handlePitchBend(byte channel, int bend) {
  if(channel == in_channel) {
    if(DEBUG){ displayPitchBend(channel, bend); }
    MIDI.sendPitchBend(bend, getOutChannel());
  } else if(channel == prog_pass_channel) {
    MIDI.sendPitchBend(bend, prog_pass_channel);
  }
}

byte getOutChannel() {
  // TODO: Switch between the dynamic and manual override channel
  return dyn_channel;
}

// Display
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

void displayPitchBend(byte channel, int bend) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Pitch Bend");
  display.println(bend);
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

