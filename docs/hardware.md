# Hardware Overview

This document lists the hardware used in INDbox and provides the GPIO mapping between the ESP32 and the connected components.

## Bill of Materials (BOM)

### Core Electronics

- **Microcontroller:** ESP32 Dev Module / NodeMCU-style ESP32 (USB-powered, Serial over USB-C)
- **Ultrasonic distance sensor:** HC-SR04
- **Push buttons:** momentary push button / tactile switch (×2)
- **Potentiometer:** 10 kΩ linear rotary potentiometer

### Passive Components

- **Decoupling capacitors:** 100 nF ceramic (recommended near ESP32 3V3 and near each HC-SR04 VCC/GND)
- **Buffer capacitor:** 47 µF electrolytic (recommended on 5V rail near the ESP32 supply)
- **Voltage divider resistors (for HC-SR04 ECHO):**
  - 1 kΩ and 2 kΩ

> Note: HC-SR04 ECHO outputs 5V logic. The ESP32 GPIOs are 3.3V-only.
> Therefore ECHO must be level-shifted using a resistor divider.

### Connectors / Wiring

- 2.54 mm pin headers (or JST connectors) for:
  - Buttons
  - Potentiometer
  - HC-SR04 sensors
- Flexible wire (0.14–0.25 mm² is typical for internal wiring)

### Mechanical

- 3D-printed enclosure (see in `/hardware`)
- Mounting hardware (M3 screws, spacers, etc.)
- Perfboard

## Power

INDbox is powered via USB-C through the ESP32 board.

- **VIN / 5V:** used to power HC-SR04 sensors
- **3V3:** used for the potentiometer reference and ESP32 logic
- **GND:** common ground for all components

Recommended decoupling:

- 47 µF electrolytic between **5V and GND** (near ESP32 VIN)
- 100 nF ceramic between **3V3 and GND** (near ESP32 3V3 pin)
- 100 nF ceramic between **VCC and GND** near HC-SR04

## GPIO Mapping (INDbox)

This is the default pin mapping used by the firmware.

![](/assets/esp32-overview.png)

### Inputs

| Component     | Signal | ESP32 Pin Label | GPIO   | Notes                                               |
| ------------- | ------ | --------------- | ------ | --------------------------------------------------- |
| Button 1      | BTN1   | D32             | GPIO32 | Uses internal pull-up (active LOW)                  |
| Button 2      | BTN2   | D33             | GPIO33 | Uses internal pull-up (active LOW)                  |
| Potentiometer | Wiper  | D34             | GPIO34 | ADC input (0–4095). Connect pot ends to 3V3 and GND |
| HC-SR04       | TRIG   | D25             | GPIO25 | Digital output                                      |
| HC-SR04       | ECHO   | D26             | GPIO26 | Digital input **via voltage divider** (5V → 3.3V)   |

## Wiring Notes

### Buttons

Buttons are connected between the GPIO and GND.
The firmware enables `INPUT_PULLUP`, so the button reads:

- **released:** 1
- **pressed:** 0 (active LOW)

The firmware inverts this so the serial output uses:

- **released:** 0
- **pressed:** 1

### Potentiometer

The potentiometer is used as a voltage divider:

- One outer pin → **3V3**
- Middle pin (wiper) → **GPIO34 (ADC)**
- Other outer pin → **GND**

Do not connect the potentiometer to 5V, as the ESP32 ADC is 3.3V max.

### HC-SR04 Level Shifting (ECHO)

ECHO must be reduced to 3.3V logic level using a resistor divider:

- HC-SR04 ECHO → **2 kΩ** → node to ESP32 ECHO GPIO
- Node → **1 kΩ** → GND

This produces approximately 3.3V at the node when ECHO is 5V.

## Firmware Data Format (Serial)

The firmware streams a continuous CSV line (default ~30 Hz):

```csv
btn1,btn2,pot,dist1,dist2
```

Where:

- `btn1`, `btn2`: 0 or 1
- `pot`: 0..4095
- `dist1`, `dist2`: distance in millimeters (or -1 if no echo)

See `/docs/flashing.md` for setup and `/firmware` for implementation details.
