# LLM Brain for Robot Pet 🧠

This folder contains the Python "Brain" that controls the ESP32 "Muscle" via USB Serial. It enables Vision, Voice, and Personality.

## Architecture
- **Mind (`server.py`)**: Runs the LLM (Ollama), Speech-to-Text (Vosk), and Text-to-Speech (Piper). It decides *what* to say and *how* to feel.
- **Body Driver (`main.py`)**: Handles Vision (MediaPipe) and manages the audio/hardware lifecycle. It tells the ESP32 where to look.
- **Spine (`utils.py`)**: Manages the Serial connection to the ESP32.

## Prerequisites
- **Hardware**: 
    - A Computer or Raspberry Pi 4/5.
    - USB Webcam & Microphone.
    - USB Speaker.
    - ESP32 Robot connected via USB.
- **Software**:
    - Python 3.9+
    - [Ollama](https://ollama.com/) running locally.

## Setup

### 1. Install Dependencies
Run the included setup script to download models (Vosk, Piper) and install Python libs:
```bash
cd llm_brain
chmod +x setup_brain.sh
./setup_brain.sh
```

### 2. Pull LLM Model
Ensure Ollama is running, then pull the model customized in `trooper_config.json` (Default: `gemma:2b` or similar):
```bash
ollama pull gemma3:1b
```

## Usage

### 1. Connect the Robot
Plug the ESP32 into USB. Ensure it is on `/dev/ttyUSB0` (or edit `utils.py`).

### 2. Run the System
You need two terminal windows:

**Terminal 1 (The Body/Vision):**
```bash
python3 main.py
```
*Starts Camera, Hand Tracking, and Audio Client.*

**Terminal 2 (The Mind/Voice):**
```bash
python3 server.py
```
*Starts LLM, Speech Recognition, and Text-to-Speech.*

## Configuration
Edit `trooper_config.json` to customize:
- `volume`: Speaker volume %
- `voice`: Piper voice model filename
- `system_prompt`: The personality of the robot (e.g., "You are a loyal stormtrooper").
- `vision_wake`: `true` to wake up when a face is seen.

## Troubleshooting
- **Permission Denied**: `sudo usermod -aG dialout $USER` to access Serial.
- **Audio Error**: Check `mic_name` and `audio_output_device` match your `aplay -l` and `arecord -l` output.
