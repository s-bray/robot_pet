import json
import os
import pyaudio
import errno
import numpy as np
import serial
import time
import threading

def load_config():
    # Load config relative to this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    CONFIG_PATH = os.path.join(script_dir, "trooper_config.json")
    DEFAULTS = {
        "volume": 95,
        "mic_name": "USB Camera-B4.09.24.1: Audio",
        "audio_output_device": "USB PnP Sound Device: Audio",
        "model_name": "gemma3:1b",
        "voice": "danny-low.onnx",
        "mute_mic_during_playback": True,
        "fade_duration_ms": 50,
        "retro_voice_fx": False,
        "history_length": 6,
        "system_prompt": "You are a loyal Imperial Stormtrooper. You need to keep order. Your weapon is a gun. Don’t ask to help or assist.",
        "greeting_message": "Identify yourself!",
        "closing_message": "Mission completed. Carry on with your civilian duties.",
        "timeout_message": "Communication terminated. Returning to base.",
        "session_timeout": 500,
        "vision_wake": False
    }

    try:
        if os.path.exists(CONFIG_PATH):
            try:
                with open(CONFIG_PATH, "r") as f:
                    cfg = json.load(f)
                    print("[Config] Loaded from file:", CONFIG_PATH)
                    return {**DEFAULTS, **cfg}
            except Exception as e:
                print("[Config] Failed to load config, using defaults:", e)
        else:
            print("[Config] Config file not found, using defaults.")
    except Exception as e:
        print(f"[Config] Error loading config: {e}")

    print("[Config] Using defaults only.")
    return DEFAULTS
    
def get_voice_sample_rate(voice_name):
    json_path = os.path.join("voices", voice_name + ".json")
    try:
        with open(json_path, "r") as f:
            meta = json.load(f)
        return meta.get("audio", {}).get("sample_rate", 16000)
    except Exception:
        return 16000  # default fallback

def list_pyaudio_devices():
    print("\n[PyAudio Devices]")
    pa = pyaudio.PyAudio()
    for i in range(pa.get_device_count()):
        info = pa.get_device_info_by_index(i)
        name = info.get("name", "Unknown")
        inputs = info.get("maxInputChannels", 0)
        outputs = info.get("maxOutputChannels", 0)
        print(f"  [{i}] {name} | in: {inputs}ch  out: {outputs}ch")
    pa.terminate()

def find_device(target_name, is_input=True):
    pa = pyaudio.PyAudio()
    target_name = target_name.lower()
    for i in range(pa.get_device_count()):
        info = pa.get_device_info_by_index(i)
        name = info.get("name", "").lower()
        if target_name in name:
            if is_input and info.get("maxInputChannels", 0) > 0:
                print(f"[Device] Found input #{i}: {info['name']}")
                pa.terminate()
                return i
            elif not is_input and info.get("maxOutputChannels", 0) > 0:
                print(f"[Device] Found output #{i}: {info['name']}")
                pa.terminate()
                return i
    pa.terminate()
    print(f"[Device] No match for {'input' if is_input else 'output'} '{target_name}', using default.")
    return None


def apply_fade(audio_bytes, fade_ms, sample_rate=48000, channels=2, apply_in=True, apply_out=True):
    if fade_ms == 0 or not (apply_in or apply_out):
        return audio_bytes

    fade_samples = int((fade_ms / 1000.0) * sample_rate)
    total_samples = len(audio_bytes) // 2  # int16 = 2 bytes

    if total_samples < 2 * fade_samples:
        return audio_bytes

    audio = np.frombuffer(audio_bytes, dtype=np.int16).copy()

    if apply_in:
        fade_in = np.linspace(0.0, 1.0, fade_samples)
        for i in range(fade_samples):
            audio[i * channels:(i + 1) * channels] = (
                audio[i * channels:(i + 1) * channels] * fade_in[i]
            ).astype(np.int16)

    if apply_out:
        fade_out = np.linspace(1.0, 0.0, fade_samples)
        for i in range(fade_samples):
            audio[-(i + 1) * channels:-(i) * channels if i > 0 else None] = (
                audio[-(i + 1) * channels:-(i) * channels if i > 0 else None] * fade_out[i]
            ).astype(np.int16)

    return audio.tobytes()

class SerialManager:
    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            with cls._lock:
                if not cls._instance:
                    cls._instance = super(SerialManager, cls).__new__(cls)
                    cls._instance._initialized = False
        return cls._instance

    def __init__(self, port='/dev/ttyUSB0', baudrate=921600):
        if self._initialized: return
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.connect()
        self._initialized = True

    def connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[Serial] Connected to {self.port} at {self.baudrate}")
            # Reset ESP32 to ensure clean state
            self.ser.setDTR(False)
            time.sleep(0.1)
            self.ser.setDTR(True)
        except Exception as e:
            print(f"[Serial] Error connecting to {self.port}: {e}")
            self.ser = None

    def send(self, command):
        if not self.ser:
            # Try reconnecting once
            self.connect()
        
        if self.ser:
            try:
                full_cmd = f"{command}\n"
                self.ser.write(full_cmd.encode('utf-8'))
                # print(f"[Serial] Sent: {command}")
            except Exception as e:
                print(f"[Serial] Send failed: {e}")
                self.ser = None

    def read_line(self):
        if self.ser and self.ser.in_waiting:
            try:
                return self.ser.readline().decode('utf-8').strip()
            except:
                return None
        return None
