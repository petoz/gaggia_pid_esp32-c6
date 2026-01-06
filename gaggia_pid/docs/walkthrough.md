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

## Using the Controller

1.  **Monitor**: Open the Serial Monitor `115200` baud. You should see "Gaggia PID Controller Starting..." followed by "IP Address: ...".
2.  **Web Dashboard**: Open the IP address in your browser.
    -   You will see current Temp, Target, and Output.
    -   Use the form to update Target Temp, Kp, Ki, Kd on the fly.
3.  **Tuning**:
    -   Start with Kp=50, Ki=0, Kd=0.
    -   If it overshoots, reduce Kp.
    -   Increase Ki slightly (e.g., 0.1) to remove steady-state error.

## Verification Checklist
- [x] Connect Hardware.
- [x] Update WiFi in `config.h`.
- [x] Flash Firmware.
- [ ] Verify Temperature reading is ~Room Temp (20-25°C).
- [ ] Verify SSR LED blinks as it heats up.
- [ ] Verify Safety Cutoff (by testing with simulated high temp if possible).
