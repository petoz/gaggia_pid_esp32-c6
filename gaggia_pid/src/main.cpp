#include "config.h"
#include "web.h"
#include <Adafruit_MAX31865.h>
#include <Arduino.h>
#include <PID_v1.h>

// Initialize MAX31865 with software SPI
Adafruit_MAX31865 max31865 =
    Adafruit_MAX31865(PIN_SPI_CS, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);

// Global Variables
float currentTemperature = 0.0;
unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL = 250; // Read every 250ms

// PID Variables
double Setpoint, Input, Output;
// PID Constants
double Kp = PID_KP, Ki = PID_KI, Kd = PID_KD;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// SSR Control Variables
int WindowSize = 1000; // 1000ms window for SSR
unsigned long windowStartTime;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Gaggia PID Controller Starting...");

  // Initialize/Configure Pins
  pinMode(PIN_SSR, OUTPUT);
  digitalWrite(PIN_SSR, LOW);

  // Initialize MAX31865
  Serial.println("Initializing MAX31865...");
  max31865.begin(MAX31865_3WIRE);

  // Initialize PID
  Setpoint = 93.0; // Default target temp (Celsius) for Espresso
  windowStartTime = millis();
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetMode(AUTOMATIC);

  // Initialize Web Interface
  setupWeb();
}

void loop() {
  unsigned long now = millis();

  // Read Temperature
  if (now - lastTempReadTime >= TEMP_READ_INTERVAL) {
    lastTempReadTime = now;
    uint16_t rtd = max31865.readRTD();
    float temp = max31865.temperature(100.0, 430.0);

    // Fault Check
    uint8_t fault = max31865.readFault();
    if (fault) {
      Serial.print("Fault 0x");
      Serial.println(fault, HEX);
      max31865.clearFault();
      currentTemperature = -999.0; // Error value
    } else {
      currentTemperature = temp;
      // Safety Cutoff
      if (currentTemperature > MAX_TEMP_SAFETY) {
        Serial.println("SAFETY CUTOFF TRIGGERED");
        // In a real safety cutoff, we might want to halt completely or ensure
        // SSR is LOW explicitly For now, setting temperature high might not be
        // enough if PID logic is not careful. We should handle safety
        // explicitly in SSR control.
      }
    }
  }

  // PID Computation
  if (currentTemperature > 0 && currentTemperature < MAX_TEMP_SAFETY) {
    Input = currentTemperature;
    myPID.Compute();
  } else {
    Output = 0; // Forced OFF if error or over temp
  }

  // SSR Control (Time Proportioning)
  if (now - windowStartTime > WindowSize) {
    windowStartTime += WindowSize;
  }

  // Safety check just in case
  if (currentTemperature > MAX_TEMP_SAFETY || currentTemperature == -999.0) {
    digitalWrite(PIN_SSR, LOW);
  } else {
    if (Output > now - windowStartTime)
      digitalWrite(PIN_SSR, HIGH);
    else
      digitalWrite(PIN_SSR, LOW);
  }

  // TODO: Handle Web/WiFi
}
