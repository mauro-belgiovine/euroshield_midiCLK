# euroshield_midiCLK
 This code is intended to use 1010Music Euroshield as a MIDI Clock > Gate converter.

## How to use
Connect a TRS stero Jack as MIDI input to the MIDI IN of 1010Music.
The CV input are not yet configured. CV ouputs act as follows:
- `OUT1` outputs a square wave with a rate equivalent to the input MIDI clock
- `OUT2` outputs a gate whenever a RESET is triggered (with a MIDI PLAY message)