# Gaggia Classic PID Controller Implementation Plan

I will implement a PID controller firmware for the Gaggia Classic Espresso machine using an ESP32-C6, MAX31865 (PT100), and SSR.

## User Review Required

> [!IMPORTANT]
> **Framework selection**: I am proposing **PlatformIO** with the **Arduino Framework** because it has excellent library support for MAX31865 and PID. If you prefer **ESP-IDF**, please let me know.

> [!NOTE]
> **Pinout**: I have proposed a pinout below. Please wire your ESP32-C6 accordingly or let me know if you have different constraints.

> [!TIP]
> **Web Interface**: I have included a basic Web Interface to monitor temperature and tune PID parameters.

## Hardware Pinout (Proposed)

**ESP32-C6** does not have fixed SPI pins like older chips, but we should use standard defaults if possible or define them clearly.

| Device | ESP32-C6 Pin (GPIO) | Function | Notes |
| :--- | :--- | :--- | :--- |
| **MAX31865 (SPI)** | | | |
| CS | GPIO 18 | SPI Chip Select | |
| MOSI | GPIO 19 | SPI MOSI | |
| MISO | GPIO 20 | SPI MISO | |
| SCK | GPIO 21 | SPI Clock | |
| RDY | - | Data Ready | Not strictly needed (polled) |
| **SSR Relay** | GPIO 4 | PWM Output | |

*Note: Please check your specific ESP32-C6 board pinout (e.g., ESP32-C6-DevKitC-1 or similar) to ensure these GPIOs are exposed.*

## Proposed Changes

I will create a new PlatformIO project in `gaggia_pid/`.

### Dependencies
- `adafruit/Adafruit MAX31865 library` (for PT100)
- `br3ttb/PID` (Standard PID library)
- `me-no-dev/ESP Async WebServer` or similar for the UI

### Modules

#### `src/main.cpp`
- Setup hardware.
- connect to WiFi (Hardcoded or WiFiManager).
- Main loop:
    - Read Temp.
    - Compute PID.
    - Control SSR.
    - Handle Web Requests.

#### `src/hardware_config.h`
- Define Pins and Constants (PID types, Sensor type).

### Features
1.  **Temperature Reading**: Read PT100 via MAX31865. Convert to Celsius.
2.  **PID Control Loop**:
    - Update every 100-200ms.
    - Input: Temperature.
    - Output: SSR Duty Cycle (Time Proportioning for SSR).
3.  **Safety**:
    - Max Temperature Cutoff (e.g., 145°C - software safety).
    - Sensor Fault Detection (MAX31865 has built-in fault flags).
4.  **Web UI**:
    - Show Current Temp, Target Temp.
    - Set Kp, Ki, Kd, Target Temp.

## Verification Plan

### Automated Tests
- **Build Verification**: `pio run` to verify successful compilation.
- **Unit Tests (Native)**: I will try to create a native test for the PID logic using `test/test_pid.cpp` if possible, running on host.
    - *Command*: `pio test -e native`

### Manual Verification
- You will need to flash the firmware to your ESP32-C6.
- **Serial Monitor**: Check logs for "Sensor Online", temperature readings, and SSR switching.
- **Web UI**: Open the IP address and verify you can change the target temperature.

### Safety Checks (User to perform)
- Verify SSR switches OFF when temperature > Target.
- **CRITICAL**: Ensure your machine has a **thermal fuse** in series with the boiler heater for hardware safety. Software can fail; hardware fuses save houses.
