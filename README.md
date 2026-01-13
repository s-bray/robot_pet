# ESP32 Robot Pet (Trooper)

An animated, sensor-driven robot companion powered by ESP32, with an optional Python AI Brain.

![Robot Eyes](https://github.com/fluxgarage/RoboEyes/raw/master/images/roboEyes.gif)

## Architecture: Hybrid Spinal Cord
This project uses a "Hybrid" architecture:
1.  **Spinal Cord (ESP32)**: Handles immediate reflexes (Touch, Shake, Pickup) and animations.
2.  **Brain (Python/PC)** (Optional): Connects via USB Serial to provide Vision (Face/Hand Tracking) and Voice (LLM Conversation).

## Hardware Setup
- **Microcontroller**: ESP32-WROOM (Classic)
- **Display**: SSD1306 OLED (I2C)
    - SDA: GPIO 21
    - SCL: GPIO 22
- **Servos**: 2x SG90
    - Left: GPIO 18
    - Right: GPIO 19
- **Sensors**:
    - Touch: GPIO 15 (TTP223 or similar)
    - IMU: GPIO 21/22 (MPU6050 sharing I2C bus)

## Features
### 1. Autonomous Reflexes (Offline)
Even without a computer connection, the robot is alive:
- **Touch**: Touching the head (GPIO 15) triggers a "Happy/Squint" animation and fast wiggle.
- **Pickup**: Picking it up triggers an "Excited" wiggle.
- **Shake**: Shaking it makes it "Dizzy".
- **Idle**: Looks around with random blinks.

### 2. AI Brain Control (Online/USB)
When connected to a computer running the `llm_brain` scripts:
- **Face Wake**: Wakes up and smiles when it sees a face.
- **Hand Tracking**: Moves servos to follow your hand.
- **Voice Chat**: Speaks and acts out emotions determined by an LLM (e.g. `[HAPPY]`, `[ANGRY]`).

## Serial Protocol (921600 Baud)
The ESP32 listens for commands on the Serial port:
| Command | Arguments | Effect |
|:---|:---|:---|
| `E:MOOD` | `HAPPY`, `ANGRY`, `SCARED`, etc. | Sets eye animation emotion. |
| `S:L:R` | `0-180` : `0-180` | Moves Left/Right servos to specific angle. |
| `W:SPEED` | `ms` | Starts wiggling at speed (lower is faster). `0` to stop. |
| `R` | None | Resets to Default state. |

## Installation (Firmware)
1.  Install **PlatformIO**.
2.  Clone this repo.
3.  Upload to ESP32:
    ```bash
    pio run -t upload
    ```

## Python Brain Setup
See [llm_brain/README.md](llm_brain/README.md) for instructions on setting up the AI features.
