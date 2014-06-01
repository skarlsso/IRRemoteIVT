# IVT Nordic Inverter Arduino IR Remote Control

Control the IVT Nordic Inverter heat pump by sending commands to the serial port of a SparkFun Pro Micro Arduino board [1]. Together with an Internet connected device, like a Raspberry Pi, this could be used to control the heat pump remotely.

### Examples of commands:
* on   - Turn on the heat pump
* off  - Turn off the heat pump
* heat - Turn on heating
* cool - Turn on cooling
* temp - Set the temperature

### Parts needed to replicate my setup:
* Pro Micro
* 940 nm IR diode (x2)
* Transistor to drive the diode(s)
* Resistors

[1] https://www.sparkfun.com/products/12640