/*
  INDbox v1 – Firmware (Prototype: 2 Buttons, 1 Potentiometer, 1x HC-SR04)

  Board:
    - ESP32 Dev Module / NodeMCU-style ESP32 (Joy-IT SBC NodeMCU ESP32-C)

  What it does:
    - Reads two push buttons (digital, with internal pull-ups)
    - Reads one 10k linear potentiometer (analog, 12-bit ADC)
    - Measures distance using one HC-SR04 ultrasonic sensor
    - Streams values as CSV over USB Serial for Processing / p5.js / etc.

  Serial format (one line per sample):
    btn1,btn2,pot,dist_mm

  Notes:
    - HC-SR04 ECHO is a 5V signal -> MUST be level-shifted to 3.3V before going into an ESP32 GPIO.
    - Buttons are wired from GPIO to GND and use INPUT_PULLUP.
*/

#include <Arduino.h>

// ----------------------------
// Pin Mapping (INDbox v1)
// ----------------------------
static const int PIN_BTN1 = 32;   // GPIO32
static const int PIN_BTN2 = 33;   // GPIO33
static const int PIN_POT  = 34;   // GPIO34 (ADC input only on ESP32)

static const int PIN_US_TRIG = 25; // GPIO25
static const int PIN_US_ECHO = 26; // GPIO26 (INPUT) - level shift required!

// ----------------------------
// Serial / Timing Configuration
// ----------------------------

// Serial baud rate: 115200 is a common, reliable default for USB serial.
static const uint32_t SERIAL_BAUD = 115200;

// Output rate: 30 Hz feels responsive for interaction and is stable for HC-SR04.
// 1000 ms / 30 ≈ 33.33 ms
static const uint32_t SAMPLE_INTERVAL_MS = 33;

// ----------------------------
// Ultrasonic Configuration
// ----------------------------

// HC-SR04 trigger pulse length in microseconds.
// Datasheet-style typical value: 10 µs HIGH pulse.
static const uint32_t US_TRIG_PULSE_US = 10;

// Small settling delay before trigger pulse (avoids spurious edges).
static const uint32_t US_TRIG_SETTLE_US = 2;

// pulseIn timeout in microseconds.
// 30,000 µs = 30 ms.
// Distance for 30 ms round-trip:
//   distance ≈ (time_us * 0.343 mm/us) / 2  ≈ 5145 mm (~5.1 m)
// So 30 ms covers the full practical range of HC-SR04 (~4 m) with margin.
static const uint32_t US_ECHO_TIMEOUT_US = 30000;

// Speed of sound in air at ~20°C:
//   343 m/s = 0.343 mm/µs
// We use mm/µs to compute millimeters directly from pulse length.
static const float SOUND_SPEED_MM_PER_US = 0.343f;

// Return value when no echo is received within timeout.
static const long DIST_NO_ECHO = -1;

// ----------------------------
// Helper: Read distance in millimeters from HC-SR04
// ----------------------------
long readUltrasonicMillimeters() {
  // Ensure a clean LOW before starting
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(US_TRIG_SETTLE_US);

  // Trigger: HIGH pulse
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(US_TRIG_PULSE_US);
  digitalWrite(PIN_US_TRIG, LOW);

  // Measure ECHO pulse width (HIGH time) in microseconds
  // pulseIn returns 0 if timeout happens
  const uint32_t duration_us = pulseIn(PIN_US_ECHO, HIGH, US_ECHO_TIMEOUT_US);
  if (duration_us == 0) {
    return DIST_NO_ECHO;
  }

  // Convert time-of-flight to distance:
  // distance = (duration_us * speed_of_sound) / 2
  // divide by 2 because the sound travels to the object and back.
  const float distance_mm = (duration_us * SOUND_SPEED_MM_PER_US) * 0.5f;

  // Round to integer millimeters
  return (long)(distance_mm + 0.5f);
}

// ----------------------------
// Setup
// ----------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);

  // Buttons: wired to GND, internal pull-up enabled.
  // Read = LOW when pressed, HIGH when released.
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);

  // Potentiometer: wiper to ADC pin (GPIO34), ends to 3V3 and GND.
  pinMode(PIN_POT, INPUT);

  // Ultrasonic sensor pins
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);

  // CSV header (optional but helpful for debugging)
  Serial.println("btn1,btn2,pot,dist_mm");
}

// ----------------------------
// Main Loop (fixed sample rate)
// ----------------------------
void loop() {
  static uint32_t lastSampleMs = 0;
  const uint32_t nowMs = millis();

  if (nowMs - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return; // wait until next tick
  }
  lastSampleMs = nowMs;

  // Buttons: invert because of INPUT_PULLUP
  // pressed -> 1, released -> 0
  const int btn1 = (digitalRead(PIN_BTN1) == LOW) ? 1 : 0;
  const int btn2 = (digitalRead(PIN_BTN2) == LOW) ? 1 : 0;

  // ESP32 analogRead is typically 12-bit: 0..4095
  const int pot = analogRead(PIN_POT);

  // Distance in mm, or -1 if no echo
  const long dist_mm = readUltrasonicMillimeters();

  // CSV output
  Serial.print(btn1);
  Serial.print(',');
  Serial.print(btn2);
  Serial.print(',');
  Serial.print(pot);
  Serial.print(',');
  Serial.println(dist_mm);
}
