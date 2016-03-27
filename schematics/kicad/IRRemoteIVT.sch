EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "IRRemoteIVT IR Diode Connection"
Date "2016-03-27"
Rev ""
Comp "Stefan Karlsson"
Comment1 "One way to connect the IR LED to the Arduino"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Q_NPN_CBE Q1
U 1 1 56F7B715
P 8100 5500
F 0 "Q1" H 8400 5550 50  0000 R CNN
F 1 "2N5551" H 8700 5450 50  0000 R CNN
F 2 "" H 8300 5600 50  0000 C CNN
F 3 "" H 8100 5500 50  0000 C CNN
	1    8100 5500
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 56F7B753
P 8200 4550
F 0 "R1" V 8280 4550 50  0000 C CNN
F 1 "100" V 8200 4550 50  0000 C CNN
F 2 "" V 8130 4550 50  0000 C CNN
F 3 "" H 8200 4550 50  0000 C CNN
	1    8200 4550
	1    0    0    -1  
$EndComp
$Comp
L LED D1
U 1 1 56F7B7B5
P 8200 5000
F 0 "D1" H 8200 5100 50  0000 C CNN
F 1 "940 nm IR LED" H 8200 4900 50  0000 C CNN
F 2 "" H 8200 5000 50  0000 C CNN
F 3 "" H 8200 5000 50  0000 C CNN
	1    8200 5000
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR01
U 1 1 56F7B86B
P 8200 5900
F 0 "#PWR01" H 8200 5650 50  0001 C CNN
F 1 "GND" H 8200 5750 50  0000 C CNN
F 2 "" H 8200 5900 50  0000 C CNN
F 3 "" H 8200 5900 50  0000 C CNN
	1    8200 5900
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR02
U 1 1 56F7B885
P 8200 4200
F 0 "#PWR02" H 8200 4050 50  0001 C CNN
F 1 "+5V" H 8200 4340 50  0000 C CNN
F 2 "" H 8200 4200 50  0000 C CNN
F 3 "" H 8200 4200 50  0000 C CNN
	1    8200 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	8200 4200 8200 4400
Wire Wire Line
	8200 4700 8200 4800
Wire Wire Line
	8200 5200 8200 5300
Wire Wire Line
	8200 5700 8200 5900
$Comp
L R R2
U 1 1 56F7BB51
P 7550 5500
F 0 "R2" V 7630 5500 50  0000 C CNN
F 1 "1k" V 7550 5500 50  0000 C CNN
F 2 "" V 7480 5500 50  0000 C CNN
F 3 "" H 7550 5500 50  0000 C CNN
	1    7550 5500
	0    1    1    0   
$EndComp
$Comp
L PWR_FLAG #FLG03
U 1 1 56F7BDDF
P 12750 500
F 0 "#FLG03" H 12750 595 50  0001 C CNN
F 1 "PWR_FLAG" H 12750 680 50  0000 C CNN
F 2 "" H 12750 500 50  0000 C CNN
F 3 "" H 12750 500 50  0000 C CNN
	1    12750 500 
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG04
U 1 1 56F7BDFB
P 13250 500
F 0 "#FLG04" H 13250 595 50  0001 C CNN
F 1 "PWR_FLAG" H 13250 680 50  0000 C CNN
F 2 "" H 13250 500 50  0000 C CNN
F 3 "" H 13250 500 50  0000 C CNN
	1    13250 500 
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR05
U 1 1 56F7BE17
P 12750 700
F 0 "#PWR05" H 12750 450 50  0001 C CNN
F 1 "GND" H 12750 550 50  0000 C CNN
F 2 "" H 12750 700 50  0000 C CNN
F 3 "" H 12750 700 50  0000 C CNN
	1    12750 700 
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR06
U 1 1 56F7BE33
P 13550 500
F 0 "#PWR06" H 13550 350 50  0001 C CNN
F 1 "+5V" H 13550 640 50  0000 C CNN
F 2 "" H 13550 500 50  0000 C CNN
F 3 "" H 13550 500 50  0000 C CNN
	1    13550 500 
	1    0    0    -1  
$EndComp
Wire Wire Line
	12750 500  12750 700 
Wire Wire Line
	13250 500  13550 500 
Wire Wire Line
	7900 5500 7700 5500
Wire Wire Line
	7400 5500 6700 5500
Text Label 6700 5500 0    60   ~ 0
ArduinoPin9
Text Notes 6650 5900 0    60   ~ 0
R1 limits the base current,\nbut still drives the transistor\ninto saturation.
Text Notes 8500 5400 0    60   ~ 0
R1 limits the current that passes \nthrough the IR LED.\n\nThe current through the LED is approximately:\n=> (5v - V_D1_drop - V_Q_CE_sat) / R1\n=> 3.5v / 100 ohm\n=> 35mA.\n\nThe power through R1 is approximately:\n=> P = U*I\n=> 3.5v * 35mA\n=> 0.1225W\n\nR1 can be lowered to get higher LED current,\nas long as the power ratings of D1 and R1 are\nnot exceeded. \n
$EndSCHEMATC
