# S.M.U. – Custom RLC Component Meter

S.M.U. is a portable RLC meter based on the Raspberry Pi Pico, developed for testing coils and passive components during electronics prototyping. Resistance is measured using a voltage divider, capacitance through RC discharge timing to approximately 37% of the initial voltage, and inductance through LC resonance and frequency analysis. Custom measurement algorithms and automatic range switching process the signals, while an SSD1306 OLED presents the results. The device combines analog measurement circuits, embedded software, a custom PCB and Li-Ion power supply in a single enclosure.

## Technologies

`Raspberry Pi Pico` `C/C++` `ADC measurements` `RC time constant` `LC resonance` `LM339 comparator` `SSD1306 OLED` `Custom PCB` `Li-Ion battery`

## Usage and development status

Select resistance, capacitance or inductance mode, place the unpowered component in the measurement connector and start the measurement. Capacitors should always be fully discharged before being connected. The prototype has been built and tested; future improvements include increased accuracy, more reliable auto-ranging, improved stability and a redesigned analog front-end.
