/*
  INDbox v1 – Firmware (2 Buttons, 1 Potentiometer, 1x HC-SR04)
  Robust filtering version: no -1 output, smooth distance stream.

  Strategy:
    1) Measure raw distance (mm) with HC-SR04.
    2) Reject invalid readings (timeout or out-of-range).
    3) Hold last valid value if the new reading is invalid.
    4) Apply EMA smoothing to reduce jitter.
    5) (Optional) Limit maximum step per sample to remove sudden jumps.

  Output CSV:
    btn1,btn2,pot,dist_mm
*/

#include <Arduino.h>

// ----------------------------
// Pin Mapping (INDbox v1)
// ----------------------------
static const int PIN_BTN1 = 32;
static const int PIN_BTN2 = 33;
static const int PIN_POT  = 34;

static const int PIN_US_TRIG = 25;
static const int PIN_US_ECHO = 26; // ECHO MUST be level-shifted to 3.3V!

// ----------------------------
// Serial / Timing
// ----------------------------
static const uint32_t SERIAL_BAUD = 115200;
static const uint32_t SAMPLE_INTERVAL_MS = 33; // ~30 Hz

// ----------------------------
// Ultrasonic constants
// ----------------------------
static const uint32_t US_TRIG_SETTLE_US   = 2;
static const uint32_t US_TRIG_PULSE_US    = 10;
static const uint32_t US_ECHO_TIMEOUT_US  = 30000;  // 30 ms ~ up to ~5 m roundtrip
static const float    SOUND_SPEED_MM_PER_US = 0.343f;

// If pulseIn times out, we treat it as "no echo"
static const long DIST_INVALID = -1;

// ----------------------------
// Filtering parameters (tune here)
// ----------------------------

// Plausible distance range for INDbox usage.
// Anything outside is treated as invalid and will not disturb the output.
static const long DIST_MIN_MM = 50;    // ignore extremely near / bad readings
static const long DIST_MAX_MM = 2000;  // typical interaction range (2 m)

// EMA smoothing factor:
// - Smaller (e.g. 0.15): smoother but more latency
// - Larger  (e.g. 0.30): more responsive but more jitter
static const float DIST_EMA_ALPHA = 0.25f;

// Optional: limit how fast the filtered distance is allowed to change per sample.
// This can eliminate sudden jumps even if a weird reading slips through.
static const bool  ENABLE_SLEW_LIMIT = true;
static const long  MAX_STEP_PER_SAMPLE_MM = 80; // max +/- change per 33ms (~2.4 m/s)

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
    return DIST_INVALID; // no echo within timeout
  }

  const float mm = (duration_us * SOUND_SPEED_MM_PER_US) * 0.5f;
  return (long)(mm + 0.5f);
}

// Validate raw distance reading
bool isDistanceValid(long mm) {
  return (mm >= DIST_MIN_MM && mm <= DIST_MAX_MM);
}

// Clamp helper for slew limiting
long clampStep(long current, long target, long maxStep) {
  long delta = target - current;
  if (delta >  maxStep) return current + maxStep;
  if (delta < -maxStep) return current - maxStep;
  return target;
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

  // CSV header (useful for debugging; Processing can ignore it if needed)
  Serial.println("btn1,btn2,pot,dist_mm");
}

// ----------------------------
// Main Loop
// ----------------------------
void loop() {
  static uint32_t lastSampleMs = 0;

  // Distance filter state:
  static bool  hasValidDistance = false; // becomes true after first valid reading
  static long  lastGoodMm = 0;           // last accepted raw reading (held on invalid)
  static float distFiltered = 0.0f;      // EMA filtered distance

  const uint32_t nowMs = millis();
  if (nowMs - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = nowMs;

  // Buttons (active LOW -> invert so pressed = 1)
  const int btn1 = (digitalRead(PIN_BTN1) == LOW) ? 1 : 0;
  const int btn2 = (digitalRead(PIN_BTN2) == LOW) ? 1 : 0;

  // Potentiometer (0..4095 typical on ESP32)
  const int pot = analogRead(PIN_POT);

  // Read raw distance
  const long rawMm = readUltrasonicMillimetersRaw();

  // 1) Range/validity check
  if (isDistanceValid(rawMm)) {
    lastGoodMm = rawMm;
    if (!hasValidDistance) {
      // First valid reading initializes the filter to avoid a long ramp-up.
      distFiltered = (float)lastGoodMm;
      hasValidDistance = true;
    }
  }
  // If invalid: keep lastGoodMm (sample-and-hold)

  // 2) Compute target distance (always a "good" value once initialized)
  long targetMm;
  if (hasValidDistance) {
    targetMm = lastGoodMm;
  } else {
    // No valid reading yet; choose a safe placeholder (0) until we have data.
    // Alternative: set to DIST_MAX_MM or keep it hidden in Processing.
    targetMm = 0;
  }

  // 3) Optional: Slew limit before smoothing
  if (ENABLE_SLEW_LIMIT && hasValidDistance) {
    long currentMm = (long)(distFiltered + 0.5f);
    targetMm = clampStep(currentMm, targetMm, MAX_STEP_PER_SAMPLE_MM);
  }

  // 4) EMA smoothing (only if initialized)
  if (hasValidDistance) {
    distFiltered = distFiltered + DIST_EMA_ALPHA * ((float)targetMm - distFiltered);
  }

  // Output integer millimeters; never prints -1
  const long outMm = hasValidDistance ? (long)(distFiltered + 0.5f) : 0;

  Serial.print(btn1);
  Serial.print(',');
  Serial.print(btn2);
  Serial.print(',');
  Serial.print(pot);
  Serial.print(',');
  Serial.println(outMm);
}
