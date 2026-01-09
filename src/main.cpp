#include "config.h"
#include "mqtt.h"
#include "web.h"
#include <Adafruit_MAX31865.h>
#include <Arduino.h>
#include <PID_v1.h>
#include <Preferences.h>

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

  // Initialize Preferences (NVS)
  Preferences preferences;
  preferences.begin("gaggia", true); // true = read-only
  Setpoint = preferences.getDouble("target", 93.0);
  Kp = preferences.getDouble("kp", PID_KP);
  Ki = preferences.getDouble("ki", PID_KI);
  Kd = preferences.getDouble("kd", PID_KD);
  preferences.end();

  Serial.printf("Loaded Settings: Target=%.1f, Kp=%.1f, Ki=%.1f, Kd=%.1f\n",
                Setpoint, Kp, Ki, Kd);

  // Initialize PID
  windowStartTime = millis();
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetTunings(Kp, Ki, Kd);
  myPID.SetMode(AUTOMATIC);

  // Initialize Web Interface
  setupWeb();

  // Initialize MQTT
  setupMQTT();
}

void loop() {
  unsigned long now = millis();

  // MQTT Handling
  handleMQTT();

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
      
      static uint8_t faultCounter = 0;
      faultCounter++;
      
      // Only report error if fault persists for > 1 second (4 readings)
      if (faultCounter > 4) {
        currentTemperature = -999.0; // Real error
      } else {
        // Transient fault - keep using last valid temperature
        Serial.println("Transient Sensor Fault - Ignoring");
      }
    } else {
      static uint8_t faultCounter = 0;
      faultCounter = 0; // Reset counter on good read
      currentTemperature = temp;
      
      // Safety Cutoff
      if (currentTemperature > MAX_TEMP_SAFETY) {
        Serial.println("SAFETY CUTOFF TRIGGERED");
         // Additional safety logic could go here
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

  // Handle Web Server Requests
  handleWebLoop();
}
