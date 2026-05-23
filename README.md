# S.M.U. – Custom RLC Component Meter

## Description

S.M.U. is a custom-built RLC component meter developed for measuring resistance, capacitance and inductance of common electronic components. The project originated from the need for a convenient way to test coils and passive components during electronics prototyping.

The measurement process is intentionally simple: the user selects the component type, places it into the measurement connector and starts the measurement.

The project combines analog measurement circuits, signal processing and embedded software into a single portable device.

## Features

* Resistance measurement
* Capacitance measurement
* Inductance measurement
* Automatic range switching
* Custom measurement algorithms
* OLED user interface
* Custom PCB
* Battery-powered operation

## Hardware

* Raspberry Pi Pico
* SSD1306 OLED display
* LM339 comparator
* Analog measurement circuits
* Custom PCB
* Li-Ion power supply

## Measurement methods

### Resistance

Resistance measurement is based on a voltage divider and reference resistors.

### Capacitance

Capacitance is measured using the RC time constant method by detecting the discharge time to approximately 37% of the initial voltage.

### Inductance

Inductance measurement is based on LC resonance and frequency analysis using a comparator and oscillation counting.

## Software

* C/C++
* ADC measurements
* Frequency measurement
* Signal processing
* User interface control

## Challenges

Main development challenges:

* extending the measurement range,
* reducing measurement error at high resistance values,
* reliable oscillation detection,
* improving measurement stability,
* optimizing analog measurement circuits.

## Current status

Working prototype built and tested.

Future improvements:

* improved accuracy,
* better auto-ranging,
* higher measurement stability,
* redesigned analog front-end.
