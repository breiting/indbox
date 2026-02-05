# INDbox

_Interaction & Generative Design Interface_

🎛️ INDbox is an open-source physical computing interface designed for creative coding, generative design, and interactive media education.

It provides a standardized hardware input platform that allows students to explore sensor-based interaction systems without needing to build electronics from scratch.

The box acts as a bridge between the physical and digital world: tangible input is captured by a microcontroller and transmitted via USB to creative coding environments such as [Processing](https://processing.org), [p5.js](https://p5js.org), [TouchDesigner](https://derivative.ca/UserGuide/TouchDesigner), [Unity](https://unity.com), or [OpenFrameworks](https://openframeworks.cc).

![](/assets/indbox.png)

## 🧠 Concept

INDbox is **not a consumer product** but an educational interface and experimentation tool.

It enables students to:

- Understand input → transformation → output systems
- Design expressive mappings between sensors and visuals
- Prototype interactive installations quickly
- Work within a shared hardware standard
- Exchange code and ideas without hardware incompatibilities

## 🔧 Hardware Overview

Each INDbox contains:

- Microcontroller: ESP32 (USB-powered, Serial over USB)
- 2 × Push Buttons (discrete input)
- 1 × Rotary Potentiometer (continuous input)
- 1 × Ultrasonic Distance Sensors (spatial input)
- Custom 3D-printed enclosure
- Internal PCB / wiring harness

All sensor data is streamed via USB Serial to a host computer.

INDbox itself produces no visual output — all feedback happens in software.

## 📦 Open Source Hardware

All hardware files are available in this repository:

- Schematics
- PCB layouts
- Bill of Materials (BOM)
- 3D enclosure files (STL / STEP)

You are free to modify, reproduce, and adapt the design.

## 🛠️ Assembly

Step-by-step [assembly guide](/docs/hardware.md):

## ⚡ Firmware Flashing

The firmware is written in Arduino (ESP32 core).

It reads all sensors and streams standardized as simple CSV data:

```
btn1,btn2,pot,dist
```

Firmware flashing instructions can be found in [flashing instructions](/docs/flashing.md)

Includes:

- Arduino IDE installation
- ESP32 board setup
- USB driver notes
- Upload instructions

## 🛒 Purchase Assembled Unit

If you prefer not to build the hardware yourself, an assembled INDbox can be ordered here:

👉 [Order INDbox](https://www.reitinger.eu/shop/indbox-8)

## 🎓 Example Projects

The INDbox is part of an educational program taught by the author at [FH Joanneum](https://www.fh-joanneum.at) and will be used during the summer semester 2026.

After the course, students are encouraged to share and publish their projects in a separate repository dedicated to INDbox-based work.

## 📜 License

Hardware and firmware are released under open-source licenses.

See LICENSE file for details.

## 🧩 Version

`INDbox v1`: Initial educational release.
