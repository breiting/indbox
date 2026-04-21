/*
  INDbox v1.1 – Firmware (2 Buttons, 1 Potentiometer, 1x HC-SR04)
  Revised version with controllable distance sensor update rate.

  Features:
    - CSV output: btn1,btn2,pot,dist_mm
    - DIST_ON  / DIST_OFF via serial
    - DIST_HZ <value> to change distance measurement rate
    - STATUS command for debugging
    - Distance stream remains filtered and never outputs -1

  Notes:
    - Potentiometer is still read raw for now.
    - Distance updates can be slowed down independently from the main output rate.
*/

#include <Arduino.h>

// ----------------------------
// Pin Mapping (INDbox v1.1)
// ----------------------------
static const int PIN_BTN1 = 33;
static const int PIN_BTN2 = 25;
static const int PIN_POT  = 35;

static const int PIN_US_TRIG = 26;
static const int PIN_US_ECHO = 32; // ECHO MUST be level-shifted to 3.3V!

// ----------------------------
// Serial / Timing
// ----------------------------
static const uint32_t SERIAL_BAUD = 115200;
static const uint32_t OUTPUT_INTERVAL_MS = 33; // ~30 Hz CSV output

// ----------------------------
// Ultrasonic constants
// ----------------------------
static const uint32_t US_TRIG_SETTLE_US     = 2;
static const uint32_t US_TRIG_PULSE_US      = 10;
static const uint32_t US_ECHO_TIMEOUT_US    = 30000;  // 30 ms
static const float    SOUND_SPEED_MM_PER_US = 0.343f;

static const long DIST_INVALID = -1;

// ----------------------------
// Distance filtering parameters
// ----------------------------
static const long  DIST_MIN_MM = 50;
static const long  DIST_MAX_MM = 2000;

static const float DIST_EMA_ALPHA = 0.25f;

static const bool ENABLE_SLEW_LIMIT = true;
static const long MAX_STEP_PER_SAMPLE_MM = 80;

// ----------------------------
// Distance runtime settings
// ----------------------------
static bool  distanceEnabled = true;
static float distanceHz = 10.0f;  // default distance measurement frequency
static uint32_t distanceIntervalMs = 100; // derived from Hz

// ----------------------------
// Distance filter state
// ----------------------------
static bool  hasValidDistance = false;
static long  lastGoodMm = 0;
static float distFiltered = 0.0f;

// ----------------------------
// Helper: update interval from Hz
// ----------------------------
void setDistanceHz(float hz) {
  if (hz < 0.5f) hz = 0.5f;
  if (hz > 30.0f) hz = 30.0f;

  distanceHz = hz;
  distanceIntervalMs = (uint32_t)(1000.0f / distanceHz + 0.5f);
  if (distanceIntervalMs < 1) distanceIntervalMs = 1;
}

// ----------------------------
// Ultrasonic raw measurement
// ----------------------------
long readUltrasonicMillimetersRaw() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(US_TRIG_SETTLE_US);

  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(US_TRIG_PULSE_US);
  digitalWrite(PIN_US_TRIG, LOW);

  const uint32_t duration_us = pulseIn(PIN_US_ECHO, HIGH, US_ECHO_TIMEOUT_US);
  if (duration_us == 0) {
    return DIST_INVALID;
  }

  const float mm = (duration_us * SOUND_SPEED_MM_PER_US) * 0.5f;
  return (long)(mm + 0.5f);
}

bool isDistanceValid(long mm) {
  return (mm >= DIST_MIN_MM && mm <= DIST_MAX_MM);
}

long clampStep(long current, long target, long maxStep) {
  long delta = target - current;
  if (delta >  maxStep) return current + maxStep;
  if (delta < -maxStep) return current - maxStep;
  return target;
}

// ----------------------------
// Serial command handling
// ----------------------------
void printStatus() {
  Serial.print("# DIST=");
  Serial.print(distanceEnabled ? "ON" : "OFF");
  Serial.print(" DIST_HZ=");
  Serial.print(distanceHz, 2);
  Serial.print(" DIST_INTERVAL_MS=");
  Serial.print(distanceIntervalMs);
  Serial.print(" LAST_GOOD_MM=");
  Serial.print(lastGoodMm);
  Serial.print(" FILTERED_MM=");
  Serial.println((long)(distFiltered + 0.5f));
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("DIST_ON")) {
      distanceEnabled = true;
      Serial.println("# DIST_ON");
    }
    else if (cmd.equalsIgnoreCase("DIST_OFF")) {
      distanceEnabled = false;
      Serial.println("# DIST_OFF");
    }
    else if (cmd.equalsIgnoreCase("STATUS")) {
      printStatus();
    }
    else if (cmd.startsWith("DIST_HZ") || cmd.startsWith("dist_hz")) {
      int spaceIndex = cmd.indexOf(' ');
      if (spaceIndex > 0) {
        String valueStr = cmd.substring(spaceIndex + 1);
        valueStr.trim();
        float hz = valueStr.toFloat();

        if (hz > 0.0f) {
          setDistanceHz(hz);
          Serial.print("# DIST_HZ=");
          Serial.println(distanceHz, 2);
        } else {
          Serial.println("# ERR invalid DIST_HZ value");
        }
      } else {
        Serial.println("# ERR usage: DIST_HZ 10");
      }
    }
    else {
      Serial.print("# ERR unknown command: ");
      Serial.println(cmd);
    }
  }
}

// ----------------------------
// Setup
// ----------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_POT, INPUT);

  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);

  setDistanceHz(distanceHz);

  Serial.println("btn1,btn2,pot,dist_mm");
  printStatus();
}

// ----------------------------
// Main Loop
// ----------------------------
void loop() {
  static uint32_t lastOutputMs = 0;
  static uint32_t lastDistanceMs = 0;

  handleSerialCommands();

  const uint32_t nowMs = millis();

  // ------------------------
  // Distance update (independent timing)
  // ------------------------
  if (distanceEnabled && (nowMs - lastDistanceMs >= distanceIntervalMs)) {
    lastDistanceMs = nowMs;

    const long rawMm = readUltrasonicMillimetersRaw();

    if (isDistanceValid(rawMm)) {
      lastGoodMm = rawMm;

      if (!hasValidDistance) {
        distFiltered = (float)lastGoodMm;
        hasValidDistance = true;
      }
    }

    if (hasValidDistance) {
      long targetMm = lastGoodMm;

      if (ENABLE_SLEW_LIMIT) {
        long currentMm = (long)(distFiltered + 0.5f);
        targetMm = clampStep(currentMm, targetMm, MAX_STEP_PER_SAMPLE_MM);
      }

      distFiltered = distFiltered + DIST_EMA_ALPHA * ((float)targetMm - distFiltered);
    }
  }

  // ------------------------
  // Main CSV output (~30 Hz)
  // ------------------------
  if (nowMs - lastOutputMs < OUTPUT_INTERVAL_MS) return;
  lastOutputMs = nowMs;

  const int btn1 = (digitalRead(PIN_BTN1) == LOW) ? 1 : 0;
  const int btn2 = (digitalRead(PIN_BTN2) == LOW) ? 1 : 0;
  const int pot  = analogRead(PIN_POT);

  const long outMm = hasValidDistance ? (long)(distFiltered + 0.5f) : 0;

  Serial.print(btn1);
  Serial.print(',');
  Serial.print(btn2);
  Serial.print(',');
  Serial.print(pot);
  Serial.print(',');
  Serial.println(outMm);
}
