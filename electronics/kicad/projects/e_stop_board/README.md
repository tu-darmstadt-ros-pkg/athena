
## General description

This PCB serves as an E-Stop that allows the main drive motors, flipper motors and arm motors to be quickly disconnected from power using electronic switches. The E-Stop can be triggered both by a physical switch and a radio remote.

## Connections and E-Stop behavior

There are three power inputs, two receiving power from the output of the Power Distribution Board providing 33V for the main drive motors and flipper motors and another input from the 24V converter for the arm motors. There are seven power outputs which connect to the motors of the robot.
- Four outputs for each of the four main drive motors
- Two outputs for the flipper motors
- One output for the robot arm
When the E-Stop is engaged, all of these outputs will be electronically disconnected from the inputs.
A connector labeled 'E-Stop' connects to the physical E-Stop button. When this button is open, the electronic switches are disabled, cutting power to the motors. Another connector with two pins labeled 'IN' and 'GND' is used to connect to the radio receiver of the remote E-Stop. There are two possible states:
- When the 'IN' pin is pulled to 3.3V relative to 'GND', the remote E-Stop is considered disengaged and the electronic switches are enabled, giving power to the motors. 
- When the 'IN' pin is floating or pulled to 'GND', the remote E-Stop is considered engaged and power is cut.
Alternatively, the remote E-Stop receiver may act as a switch itself. If this is the case, the two pins labeled 'IN -> 3.3V' on the PCB can be bridged which will pull the 'IN' pin to 3.3V unless it is pulled to 'GND' by the remote E-Stop receiver.

**Important:**
If a remote E-Stop is connected, then the pins labeled 'EN_MC' must be bridged using a jumper. If there is no remote E-stop connected, the pins labeled 'V3_3' must be bridged.

## Communication interfaces

The main communication interface of the E-Stop board is the USB connection to the STM32 NUCLEO microcontroller. This interface can be used to read the actual E-Stop state as well as the currents from the inputs to each of the outputs.
The E-Stop board also has interfaces for I2C and SPI, which can be used to communicate with the board using another microcontroller. Currently, these interfaces are unused.

## Devices on the PCB

- STM32 NUCLEO microcontroller
	- This microcontroller reads the state of the remote E-Stop and produces a signal to the switches. It also reads the current through each switch.
- BTH500151LUAAUMA1
	- These are high side power switches, that connect or disconnect the inputs and outputs. They are controlled via a digital signal from the microcontroller and output the current flowing through them using an analog voltage.

## List of components

 - Passive components
	- Resistors
		- 3x 1kΩ 0805
		- 6x 2kΩ 0805
		- 5x 4.7kΩ 0805
		- 1x 100kΩ 0805
	- Ceramic capacitors
		- 2x 1uF 0805
		- 9x 10uF 0805
	- Electrolytic capacitors
		- 1x 68uF Size 6.3x7.7mm
		- 1x 120uF Size 6.3x7.7mm
	- Diodes
		- 2x SL26PL-TP
		- 1x SS12
		- 3x LED 0805
	- Inductors
		- 1x 100uH ASPIAIG-S6055
	
- Connectors
	- 3x Amass XT30U-M
	- 7x Amass XT30U-F
	- 2x JST XH B2B XH-A
	- 1x JST XH B3B XH-A
	- 1x JST XH B5B XH-A

- ICs
	- 6x BTH500151LUAAUMA1
	- 1x LM2594MX-5.0/NOPB

- Other devices
	- STM32 NUCLEO-L432KC

## Note

This documentation refers to the second generation E-Stop board.