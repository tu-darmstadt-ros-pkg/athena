
## General description

This board serves as a carrier for the H60SB0A050NRDC DC-DC converter which is used to power the robot arm and the Wi-Fi access point.

## Connections

There is a single 33V input that connects directly to the output of the Power Distribution Board. There are two high power 24V outputs, one of which connects to the 24V input of the E-Stop board. Another low power 24V output is connected to the Wi-Fi access point.

## Communication interface

The Arm Power Board provides an I2C interface which can be used to program the DC-DC converter and read its voltages and currents. Currently this interface is unused.

## Devices on the PCB

- MIC5282-3.3YMM-TR
	- This is a 3.3V linear regulator that provides a voltage for the power LED and the I2C interface.
- H60SB0A050NRDC
	- This is a powerful programmable DC-DC converter that can output a stable 24V given a wide input range. It is even capable of producing its 24V voltage output given a lower voltage input.
    
## List of components

- Passive components
	- Resistors
		- 1x 1kΩ DIN-0204
		- 3x 4.7kΩ DIN-0204
		- 1x 10kΩ DIN-0204
		- 1x 200kΩ DIN-0204
	- Ceramic capacitors
		- 3x 100pF THT P5.0mm
		- 1x 100nF THT P5.0mm
		- 1x 1uF THT P5.0mm
		- 1x 2.2uF THT P5.0mm
		- 1x 10uF THT P5.0mm
	- Electrolytic capacitors
		- 2x 470uF THT D10.0mm P5.0mm
	- Diodes
		- 1x LED THT D3.0mm

- Connectors
	- 1x Amass XT30U-M
	- 1x Amass XT30U-F
	- 1x JST XH B2B XH-A
	- 1x JST XH B4B XH-A

- ICs
	- 1x MIC5282-3.3YMM-TR

- Other devices
	- 1x H60SB0A050NRDC

## Note

This is the only PCB currently inside the robot that still belongs to the first generation of PCBs which used THT components.