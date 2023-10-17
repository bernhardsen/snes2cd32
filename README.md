# SNES2CD32 Adapter
This is a hobby project of mine that was never 100% completed, but it works for the most part in its current state.
`board` director contains the eagle files to create the PCB, and schematics with parts.
There are two current "bugs" with the PCB:
- pin 11 on IC1 should be pulled high through a 10k resistor (it's currently not connected)
- The two resistors on each side of C5 should be connected to VCC instead of GND

The chip used is an atmega8, and you need to put the arduino bootloader on it before transfering the firmware.

## Usage

To use the controller normally, just plug in the controller and you’re good.

### Re-map up/down
Most Amiga games would have you press up to jump. This is awkward and weird, even more so on a joypad. To remap up to a different button do the following:
1.  Press SELECT to enter configuration mode.
2.  Press UP to specify you want to remap UP, or DOWN if you want to remap DOWN.
3.  Press the button you want to swap with (A, B, X, Y, L or R).

### Turbo fire
You can enable turbo fire on A, B, X, Y, L and/or R, by doing the following:
1.  Press SELECT to enter configuration mode.
2.  Press START to specify you want to configure turbo fire.
3.  Press and hold the buttons you want to enable turbo fire on, and while holding them, press SELECT to confirm.

### Record/Playback macro
You can record a macro and have the controller automatically replay the inputs.

To start recording press SELECT, then L to specify you want to record a macro. The macro start recording on the next button press. It records until you press SELECT again, or the memory is full. The memory can fit around 50 button presses.

To play back a macro, press SELECT then R. The controller is not usable until the macro is done. Press SELECT while the macro is running to abort.

Timing is not 100% consistant in the current firmware.