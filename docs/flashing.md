# Flashing Instructions

This guide explains how to install the required software, prepare your system for ESP32 development, and flash the INDbox v1 firmware onto the microcontroller.

The firmware is located in this repository under:

`/firmware/indbox_v1.ino`

## Install Arduino IDE

Before getting started you have to install the latest Arduino IDE on your system.
The software should be available for macOS, Linux, and Windows.
Download the package from [here](https://www.arduino.cc/en/software) or use your local package manager (e.g. Homebrew on macOS: `brew install arduino-ide`).

After installation, launch the Arduino IDE once to complete the initial setup.

![](./docs/arduino-welcome.png)

## Prepare ESP32 Board Support

The INDbox uses an ESP32 microcontroller by Joy-it ([NodeMCU ESP32-C](https://joy-it.net/de/products/SBC-NodeMCU-ESP32-C)). According to their [documentation](https://joy-it.net/files/files/Produkte/SBC-NodeMCU-ESP32-C/SBC-NodeMCU-ESP32-C_Manual-EN_2025-01-17.pdf), you have to install the proper packages for the Arduino IDE.

### Step 1 - Open Preferences

In Arduino IDE:

Arduino IDE → Settings / Preferences

Find the field:

“Additional Boards Manager URLs”

Add the following URL:

`https://dl.espressif.com/dl/package_esp32_index.json`

If other URLs already exist, separate them with commas.

Click OK to save.

### Step 2 - Install ESP32 Boards

Go to:

Tools → Board → Boards Manager

Search for:

esp32

Install:

Select `esp32 by Espressif Systems`

This package includes:
• ESP32 toolchains
• USB flashing tools
• Board definitions

> HINT: Do not select the versions `3.x.x` (these one did not work for me!), but select the version `2.0.17`)

Installation may take several minutes due to package size.

### Step 3 - Connect INDbox Hardware

1.  Connect the INDbox to your computer via USB-C.
2.  The ESP32 should power on immediately.
3.  Your system will create a serial device.

Typical port names:

• macOS: /dev/cu.usbserial... or /dev/cu.SLAB_USBtoUART
• Windows: COM3, COM4, etc.
• Linux: /dev/ttyUSB0

### Step 4 - Select Board and Port

In Arduino IDE:

**Board**

Tools → Board → ESP32 Arduino → ESP32 Dev Module

(This matches the ESP32 NodeMCU-style board used in INDbox)

**Port**

Tools → Port → select the USB serial device

> Tip: Unplug the box, check the list, plug it in again — the new port is the correct one.

### Step 5 - Open Firmware

Open the firmware file:

File → Open
`./firmware/indbox_v1.ino`

The firmware reads all sensors and streams CSV data via USB Serial:

```csv
btn1,btn2,pot,dist1
```

### Step 6 - Compile and Upload

Click:

Upload → (Arrow icon)

The IDE will:

1. Compile the firmware
2. Connect to the ESP32
3. Flash the firmware

Typical console output:

Connecting...
Chip is ESP32
Uploading stub...
Writing at 0x00010000...
Hard resetting via RTS pin...

> HINT: If upload fails, press the BOOT button on the ESP32 while uploading (rarely needed on NodeMCU boards).

### Step 7 - Verify Operation

Open:

Tools → Serial Monitor

Set baud rate:

115200

You should see a continuous data stream similar to:

0,0,1234,250
1,0,1300,245

These are live sensor readings from the INDbox.

### Step 8 - Troubleshooting

No Port Appears

- Install USB drivers (CP210x / CH340 depending on board)
- Reconnect device

Upload Fails

- Check correct board selection
- Verify port
- Close Serial Monitor before uploading

No Sensor Data

- Check wiring
- Verify firmware version
- Ensure power via USB

### Step 9 - Firmware Updates

Future firmware releases will follow version naming:

`indbox_v1.x`

To update:

1. Pull latest repository version
2. Open updated firmware
3. Upload again
