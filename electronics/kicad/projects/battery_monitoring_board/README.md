
## General description

This PCB is used to monitor the cell voltages of two 4s LiPo batteries connected in series. This is done using an 8-channel analog digital converter which connects to each cell via a voltage divider.

## Connections

There is a 5V power input that connects to the 5V output from the Power Distribution Board. The power input is forwarded to an output connector on the PCB that allows daisy-chaining multiple boards.
The cell connectors of the two batteries are connected to the board via two five pin connectors.

## Communication interface

The Battery Monitoring Board provides an SPI interface which allows a microcontroller to read the cell voltage measurements from the analog digital converter. This interface currently connects to the Power Distribution Board.

## Devices on the PCB

- LD1117S33TR
	- This is a linear regulator that produces a 3.3V output from a 5V input. The output is used to power the analog digital converter.
- MCP3208T-CI/SL
	- This is an 8-channel analog digital converter that outputs its measurements via SPI.
    
## List of components

- Passive components
	- Resistors
		- 8x 1kΩ 0805
		- 1x 1kΩ 1206
		- 8x 22kΩ 0805
	- Ceramic capacitors
		- 3x 1uF 0805
		- 1x 10uF 0805
	- Electrolytic capacitors
		- 1x 120uF 6.3x7.7mm
	- Diodes
		- 1x LED 0805

- Connectors
	- 2x JST EH B2B EH-A
	- 3x JST XH B5B XH-A

- ICs
	- 1x LD1117S33TR
	- 1x MCP3208T-CI/SL

## Note

The two battery cell connectors on this PCB are connected in series, so that the highest pin of the first connector is directly connected to the lowest pin of the second connector. It is crucial that the main power connectors of the batteries are also connected in series externally.