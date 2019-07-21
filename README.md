# MIDI Channel Mapper

The MIDI Channel Mapper allows you to map any input MIDI channel to any output MIDI channel with a user-friendly interface.  Multiple input channels can map to the same output channel, and there are preset modes for directing all input channels to a single output channel and disabling all output.

## Bill of Materials

| Qty | Part                                   | Label             |
|-----|----------------------------------------|-------------------|
| 1   | SPST Switch                            | PWR_SW            |
| 1   | DPST Switch                            | MIDI_DISABLE_SW   |
| 1   | 4.7kohm Resistor                       | R1                |
| 4   | 220ohm Resistor                        | R2, R3, R4, R5    |
| 1   | 1N914 Diode                            | D1                |
| 1   | 6N138 Opto-isolator                    | U1                |
| 1   | PJ-002A Barrel Jack Power Connector    | DC1               |
| 1   | HD16K33 7-Segment Display and Backpack | DISP              |
| 2   | 10kohm Potentiometer (ALPS RK203)      | POT_IN, POT_OUT   |
| 1   | SPST Pushbutton                        | SAVE_BTN          |
| 2   | SDS-50J Female MIDI Connector          | MIDI_IN, MIDI_OUT |
| 1   | Arduino Nano (or clone)                | NANO_1, NANO_2    |

## Aruino Library Dependencies

* MIDI.h
* Wire.h
* Adafruit_GFX.h
* Adafruit_LEDBackpack.h

## Contributors

* [Jacob Bates](https://github.com/bagofarms)
* [Zach Berry](https://github.com/zachberry)
* [Shea Silverman](https://github.com/SheaSilverman)
