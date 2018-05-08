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

// MIDI Constants
#define COMMAND_PROGRAMCHANGE 0xC
#define COMMAND_AFTERTOUCH 0xD
#define MASK_CHANNEL 0x0F

int command;
int data1;
int data2;

void setup() {
  Serial.begin(31250);

  // OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();  // You must do this after every draw action
}

void loop() {
  // Write values to screen
  //writeOLED(chanIn, chanOut);
  if (Serial.available() > 2){
    command = Serial.read();//read first byte
    data1 = Serial.read();//read next byte

    // Error handling
    if(command < 127){
      command = 127;
    }

    if(data1 < 0){
      data1 = 0;
    }

    // Split the command byte
    int op = (command >> 4);

    switch(op) {
      case COMMAND_PROGRAMCHANGE:
        data2 = 0;
        handleProgramChange(command, data1);
        break;

      case COMMAND_AFTERTOUCH:
        // if it's aftertouch (0xD), so don't read the second data byte
        data2 = 0;
        handleAftertouch(command, data1);
        // Don't do anything else in the loop when aftertouch is happening
        break;

      default:
        // Since it's not a special case, read the second data byte
        data2 = Serial.read();//read final byte
      
        if(data2 < 0){
          data2 = 0;
        }
        
        handleMidi(command, data1, data2);
        // Do extra stuff here if you want to
    }
  }
}

void handleProgramChange(int command, int data1) {
  // Do something special here like changing our output channel?
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("ProgramChg");
  display.println(command);
  display.println(data1);
  display.display();
  Serial.write(command);
  Serial.write(data1);
}

void handleAftertouch(int command, int data1) {
  // Don't do anything here because aftertouch sends data too fast
  // and it will bog down the serial reads.
  Serial.write(command);
  Serial.write(data1);
}

void handleMidi(int command, int data1, int data2) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(command);
  display.println(data1);
  display.println(data2);
  display.display();
  Serial.write(command);
  Serial.write(data1);
  Serial.write(data2);
}
