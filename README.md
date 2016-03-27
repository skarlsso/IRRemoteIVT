## IVT Nordic Inverter Arduino IR Remote Control

Control the IVT Nordic Inverter heat pump by sending commands to the serial port of an ATmega32U4 Arduino board.

Together with an Internet connected device, like a Raspberry Pi, this could also be used to control the heat pump remotely.

### Examples of commands:
* help - Print the help text
* on   - Turn on the heat pump
* off  - Turn off the heat pump
* heat - Turn on heating
* cool - Turn on cooling
* temp - Set the temperature

### Building
The provided scripts use PlatformIO to compile and upload to the Arduino:
* ./build-full.sh
* ./upload-full.sh

The sketch can also be used with the Arduino IDE.

### Parts needed to replicate my setup:
* ATmega32U4 Arduino board (E.g. Micro, Leonardo, Pro Micro)
* 940 nm IR diode (E.g. TSAL4400, EL-IR323)
* Transistor to drive the IR diode (E.g. 2N5551) 
* Resistors (1k ohm, 100 ohm)

### Pin Mapping
* Pin 9 drives the LED pulses
* By default Serial is used for communication. Can be changed in serial.h.
  * On Arduino Micro, this means the serial port through the USB, by default. Change it to Serial1 to use Pin 0/1.

### Schematic
* https://github.com/skarlsso/IRRemoteIVT/blob/master/schematics/kicad/IRRemoteIVT-schematic.png
