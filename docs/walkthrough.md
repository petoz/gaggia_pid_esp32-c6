# Gaggia PID Controller Walkthrough

I have implemented a complete PID controller for your Gaggia Classic using ESP32-C6, MAX31865, and an SSR.

## Project Structure
- `gaggia_pid/src/main.cpp`: Core logic (Loop, Sensor, PID, SSR).
- `gaggia_pid/src/web.cpp`: Web Interface handling.
- `gaggia_pid/include/config.h`: Configuration (Pins, WiFi, Constants).

## Getting Started

### 1. Hardware Setup (Pinout)
Wire your ESP32-C6 as follows:

| Device | ESP32-C6 Pin | Function |
| :--- | :--- | :--- |
| **MAX31865** | | |
| CS | GPIO 14 | SPI Chip Select |
| MOSI | GPIO 15 | SPI MOSI |
| MISO | GPIO 18 | SPI MISO |
| SCK | GPIO 19 | SPI Clock |
| **SSR** | GPIO 20 | SSR Control + |

### 2. Sensor Wiring (3-Wire PT100)
Based on your module photo (Black/Purple board), follow these modification steps:

1.  **Top Jumpers (Labeled "2/3 Wire" and "2 Wire")**:
    -   **Right Jumper ("2 Wire")**: Use a sharp knife to **CUT** the thin trace connecting the two pads. Checks with a multimeter that there is no connection.
    -   **Left Jumper ("2/3 Wire")**: **SOLDER** the two pads together to close the connection.

2.  **Middle Jumpers (Labeled "24" and "3")**:
    -   You will see a block of 3 pads. Currently, the left side ("24") is connected.
    -   **Desolder/Cut** the trace on the "24" side (left).
    -   **Solder** the "3" side (right pads) to bridge them.

3.  **Terminal Connections**:
    -   **F+** and **RTD+**: Connect these two terminals together (short them with a small wire).
    -   **Red Wire**: Connect to **RTD+**.
    -   **White Wire 1**: Connect to **RTD-**.
    -   **White Wire 2**: Connect to **F-**.

*Ensure you have a thermal fuse in hardware for safety!*

### 3. Configuration & WiFi
This firmware uses **WiFiManager**.

1.  **First Boot**: When you turn on the Gaggia, the ESP32 will create a WiFi Hotspot named **GaggiaPID_Setup**.
2.  **Connect**: Connect to this hotspot with your phone or laptop.
3.  **Configure**: A portal should open automatically (or go to `192.168.4.1`). Select your home WiFi and enter the password.
4.  **Save**: The ESP32 will save the credentials and restart, connecting to your home WiFi automatically from now on.

### 4. Build and Flash
Use PlatformIO to build and upload.
> [!NOTE]
> We use the `pioarduino` platform fork to support ESP32-C6 properly with Arduino Core 3.0.

```bash
cd gaggia_pid
# If you are using VS Code, use the PlatformIO extension buttons.
# If using CLI:
pio run -t upload
pio device monitor
```

## Features & Usage

### 1. Web Dashboard
Open the IP address displayed in the Serial Monitor (e.g., `http://192.168.1.x`) in your browser.
- **Status**: View current Temp, Target, and PID Output.
- **Tuning**: Update Target Temp, Kp, Ki, Kd in real-time.

### 2. MQTT & Home Assistant
To enable integration:
1.  Go to the Web Dashboard.
2.  Enter your **MQTT Server IP**, **Port** (default 1883), **User**, and **Password**.
3.  Click **Update & Restart**.

**Home Assistant Auto-Discovery:**
- The device will automatically appear in Home Assistant as **Gaggia PID**.
- Entities created:
    - **Climate**: `climate.gaggia_pid` (Control Target Temp).
        - **Heat**: PID Active.
        - **Off**: PID Inactive, Output 0 (SSR LED Off).
    - **Number**: `number.gaggia_kp`, `ki`, `kd` (Tune PID from HA).
    - **Sensor**: `sensor.gaggia_output` (Monitor PWM Output).

![Gaggia PID Climate Entity](docs/images/ha_climate.jpg)
*Climate Entity Control*

![Gaggia PID Device Info](docs/images/ha_device.jpg)
*Device Info & Tuning Controls*

### 3. OTA Updates (Over-The-Air)
Update firmware without a USB cable:
1.  In PlatformIO, run `pio run` to build `firmware.bin` (located in `.pio/build/esp32-c6-devkitc-1/`).
2.  Go to `http://<IP_ADDRESS>/firmware`.
3.  Select the `.bin` file and click **Update Firmware**.
4.  The device will flash and restart automatically.

## Verification Checklist
- [x] Connect Hardware.
- [x] Connect to 'GaggiaPID_Setup' WiFi to configure credentials.
- [x] Flash Firmware.
- [ ] Verify Temperature reading is ~Room Temp (20-25°C).
- [ ] Verify SSR LED blinks as it heats up.
- [ ] Verify Safety Cutoff.
