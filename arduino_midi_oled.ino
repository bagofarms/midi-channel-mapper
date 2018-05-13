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

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();  // You must do this after every draw action
}

void loop() {
  // Write values to screen
  //writeOLED(chanIn, chanOut);
  MIDI.read();
}

void handleProgramChange(byte channel, byte number) {
  // Do something special here like changing our output channel?
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("ProgramChg");
  display.println(channel);
  display.println(number);
  display.display();
  MIDI.sendProgramChange(channel, number);
}

void handleAftertouch(byte command, byte data1) {
  // Don't do anything here because aftertouch sends data too fast
  // and it will bog down the serial reads.
  Serial.write(command);
  Serial.write(data1);
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(channel);
  display.println(note);
  display.println(velocity);
  display.display();
  MIDI.sendNoteOn(channel, note, velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(channel);
  display.println(note);
  display.println(velocity);
  display.display();
  MIDI.sendNoteOff(channel, note, velocity);
}
