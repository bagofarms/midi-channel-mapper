#include <MIDI.h>
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment matrix = Adafruit_7segment();

// Potentiometer inputs
#define MAX_POT_VALUE 1024
#define CHAN_IN_POT 2
#define CHAN_OUT_POT 3

#define DEBUG true

MIDI_CREATE_DEFAULT_INSTANCE();

byte in_pot_channel = 0;
byte out_pot_channel = 0;
byte last_in_pot_channel = 0;
byte last_out_pot_channel = 0;
bool is_out_pot_dirty = false;

// @TODO - IDEA. On startup hold down the SAVE Button and power on.
// In that case the default is all channels will always be routed to "--".

// The hardware contains two knobs, a 4-segment LED and a button, and is setup as follows:
// ------------------
// | [00.--.]       |
// | (I) (O) [SAVE] |
// ------------------
// * The first knob (I) is the INPUT SELECTOR.
//    * It has 17 steps labeled: OMNI, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
// * The second knob (O) is the OUTPUT SELECTOR.
//    * It has 17 setps labeled:  OFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
// * The first two digits on the LCD are the INPUT CHANNEL.
// * The first dot on the LCD is the ACTIVE CHANNEL INDICATOR.
// * The second two digits on the LCD  are the OUTPUT CHANNEL.
// * The second dot on the LCD is the OUTPUT CHANNEL DIRTY INDICATOR.
// * The save button is the... SAVE BUTTON.
// The user can turn the INPUT SELECTOR to select the INPUT CHANNEL,
// then turn the OUTPUT SELECTOR to set the OUTPUT CHANNEL.
// This will route all messages on the INPUT CHANNEL to the selected OUTPUT CHANNEL,
// however this re-routing will not take effect until it is saved with the SAVE BUTTON.
// **There are two special cases:
// A. When the INPUT CHANNEL is set to "00" by default the OUTPUT CHANNEL is set to "--" (17).
//    This is NORMAL MODE. If the user changes the OUTPUT CHANNEL to some value other than
//    "--" while the INPUT CHANNEL is "00" then this is OMNI MODE. In OMNI MODE *all*
//    messages on *all* channels will be sent to the OUTPUT CHANNEL. Setting the
//    OUTPUT CHANNEL back to "--" when the INPUT CHANNEL is "00" returns to NORMAL MODE.
// B. When the OUTPUT CHANNEL is set to "--" this *disables* the output for the given
//    INPUT CHANNEL.
// When the user changes the INPUT SELECTOR the routing for the INPUT CHANNEL and
// OUTPUT CHANNEL are shown. If the user changes the OUTPUT SELECTOR then the
// DIRTY INDICATOR is lit and the OUTPUT CHANNEL display updates. Use the SAVE BUTTON
// to write this new routing. Changing the INPUT SELECTOR to a different INPUT CHANNEL
// will disregard any pending new routing and will reset the DIRTY INDICATOR to off.
// If the user is on INPUT CHANNEL "00" then the ACTIVE CHANNEL INDICATOR will light up
// if a MIDI message is being recieved on ANY midi channel. Otherwise, the ACTIVE CHANNEL
// INDICATOR will display if the current INPUT CHANNEL displayed is getting midi messages.

// Array to hold inputs and outputs.  Index is input channel, value is output channel.
// 0 = No output, 1 = Midi Channel 1, ..., 16 = Midi Channel 16
byte channel_map[17] = {MIDI_CHANNEL_OFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// Array to keep track of which channels are active. We use this information to display
// a dot in the LED display next to the input channel (so the user can see visually
// which channels are getting messages)
bool active_map[17] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};

// Stuff that only runs every so often
#define LOOP_DELAY 500
int loop_ticks = 0;

// Button
#define BUTTON_PIN 4
int button_state;

void setup()
{
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
  matrix.drawColon(false);
  matrix.writeDigitNum(0, 0, false);
  matrix.writeDigitNum(1, 0, false);
  matrix.writeDigitNum(3, 0, false);
  matrix.writeDigitNum(4, 0, false);
  matrix.writeDisplay();
  delay(500);
  displayChannels();
}

void loop()
{
  MIDI.read();

  button_state = digitalRead(BUTTON_PIN);
  if (button_state == LOW)
  {
    updateChannels();
  }

  // Run the pot inputs and display channels every 100 loops
  if (++loop_ticks == LOOP_DELAY)
  {
    displayChannels();
    getPotInputs();
    resetActiveStateForAllChannels();

    loop_ticks = 0;
  }
}

byte getOutChannel(byte in_channel)
{
  // The OMNI channel output takes first prioirty. If it's set
  // then send ALL messages to that channel!
  if (isOMNIMode())
  {
    return channel_map[MIDI_CHANNEL_OMNI];
  }

  return channel_map[in_channel];
}

bool isOMNIMode()
{
  return channel_map[MIDI_CHANNEL_OMNI] != MIDI_CHANNEL_OFF;
}

// -------------------------------------------
// MIDI Handlers:
// -------------------------------------------

void handleProgramChange(byte channel, byte number)
{
  markChannelAsActive(channel);
  MIDI.sendProgramChange(number, getOutChannel(channel));
}

void handleNoteOn(byte channel, byte note, byte velocity)
{
  markChannelAsActive(channel);
  MIDI.sendNoteOn(note, velocity, getOutChannel(channel));
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
  markChannelAsActive(channel);
  MIDI.sendNoteOff(note, velocity, getOutChannel(channel));
}

void handlePitchBend(byte channel, int bend)
{
  markChannelAsActive(channel);
  MIDI.sendPitchBend(bend, getOutChannel(channel));
}

void handleControlChange(byte channel, byte number, byte value)
{
  markChannelAsActive(channel);
  MIDI.sendControlChange(number, value, getOutChannel(channel));
}

void handleAfterTouchPolyPressure(byte channel, byte note, byte pressure)
{
  markChannelAsActive(channel);
  MIDI.sendAfterTouch(note, pressure, getOutChannel(channel));
}

void handleAfterTouchChannelPressure(byte channel, byte pressure)
{
  markChannelAsActive(channel);
  MIDI.sendAfterTouch(pressure, getOutChannel(channel));
}

void handleTimeCodeQuarterFrame(byte data)
{
  MIDI.sendTimeCodeQuarterFrame(data);
}

void handleSongPosition(unsigned beats)
{
  MIDI.sendSongPosition(beats);
}

void handleSongSelect(byte songnumber)
{
  MIDI.sendSongSelect(songnumber);
}

void handleTuneRequest()
{
  MIDI.sendTuneRequest();
}

// Real time messages (Messages that don't specify channels)
// (These messages don't have their own sendX method, instead we must use a
// lower-level send method from the library):
void handleClock()
{
  MIDI.sendRealTime(midi::Clock);
}

void handleStart()
{
  MIDI.sendRealTime(midi::Start);
}

void handleContinue()
{
  MIDI.sendRealTime(midi::Continue);
}

void handleStop()
{
  MIDI.sendRealTime(midi::Stop);
}

void handleActiveSensing()
{
  MIDI.sendRealTime(midi::ActiveSensing);
}

void handleSystemReset()
{
  MIDI.sendRealTime(midi::SystemReset);
}

// -------------------------------------------
// Manual input methods:
// -------------------------------------------

void getPotInputs()
{
  // In pot should read OMNI (0), then 1 to 16
  in_pot_channel = normalizePotInput(analogRead(CHAN_IN_POT));
  // Out pot should read OFF, then 1 to 16
  out_pot_channel = normalizePotInput(analogRead(CHAN_OUT_POT));

  // If the in pot has changed then reset the out pot dirty state
  // (in other words, any un-saved out pot value is discarded)
  if (is_out_pot_dirty && in_pot_channel != last_in_pot_channel)
  {
    is_out_pot_dirty = false;
  }
  // If the out pot has changed we set the dirty flag, meaning the
  // user has rotated the pot and is wanting to set the out channel
  else if (!is_out_pot_dirty && out_pot_channel != last_out_pot_channel)
  {
    is_out_pot_dirty = true;
  }
  // If the out pot has changed back to the currently set value
  // of the input, the out pot is not dirty anymore
  else if (is_out_pot_dirty && out_pot_channel == channel_map[in_pot_channel])
  {
    is_out_pot_dirty = false;
  }

  last_in_pot_channel = in_pot_channel;
  last_out_pot_channel = out_pot_channel;
}

int normalizePotInput(float rawIn)
{
  rawIn = rawIn / MAX_POT_VALUE;
  rawIn = rawIn * 16;
  return (int)(rawIn + 0.5f);
}

// -------------------------------------------
// Display methods:
// -------------------------------------------

void displayChannels()
{
  // We display the left dot if we got a MIDI message on that channel:
  displayLeft(in_pot_channel, isChannelActive(in_pot_channel));

  // If the out pot has been rotated (is_out_pot_dirty=true) then we display
  // the current actual out pot value along with the 'dirty' dot.
  if (is_out_pot_dirty)
  {
    displayRight(out_pot_channel, true);
  }
  // Otherwise, show what the current mapping is for the given in pot channel.
  else
  {
    displayRight(getOutChannel(in_pot_channel), false);
  }
}

void displayLeft(byte val, bool dot)
{
  int leftDigit = val / 10;
  if (leftDigit == 0)
  {
    matrix.writeDigitRaw(0, 0B000000000);
  }
  else
  {
    matrix.writeDigitNum(0, (val / 10), false);
  }
  matrix.writeDigitNum(1, val % 10, dot);
  matrix.writeDisplay();
}

// NOTE: MIDI_CHANNEL_OFF is actually 17, but we want to display that
// on the LCD as "--".
void displayRight(int val, bool dot)
{
  if (val == MIDI_CHANNEL_OFF)
  {
    //@TODO - DISPLAY "--"
  }

  int leftDigit = val / 10;
  if (leftDigit == 0)
  {
    matrix.writeDigitRaw(3, 0B000000000);
  }
  else
  {
    matrix.writeDigitNum(3, (val / 10), false);
  }
  matrix.writeDigitNum(4, val % 10, dot);
  matrix.writeDisplay();
}

void updateChannels()
{
  channel_map[in_pot_channel] = out_pot_channel;
  is_out_pot_dirty = false;
}

void markChannelAsActive(byte channel)
{
  active_map[MIDI_CHANNEL_OMNI] = true;
  active_map[channel] = true;
}

bool isChannelActive(byte channel)
{
  return active_map[channel];
}

void resetActiveStateForAllChannels()
{
  active_map[MIDI_CHANNEL_OMNI] = false;
  active_map[1] = false;
  active_map[2] = false;
  active_map[3] = false;
  active_map[4] = false;
  active_map[5] = false;
  active_map[6] = false;
  active_map[7] = false;
  active_map[8] = false;
  active_map[9] = false;
  active_map[10] = false;
  active_map[11] = false;
  active_map[12] = false;
  active_map[13] = false;
  active_map[14] = false;
  active_map[15] = false;
  active_map[16] = false;
}
