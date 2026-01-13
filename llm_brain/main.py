# main.py
import time
import os
import threading
import subprocess
import json
from gpiozero.pins.lgpio import LGPIOFactory
from gpiozero import Device, Button
from signal import pause
import cv2
import mediapipe as mp
import wave
import io
import pyaudio
from utils import load_config, find_device, get_voice_sample_rate, SerialManager
import asyncio
import aiohttp
import glob, shutil

Device.pin_factory = LGPIOFactory()


BUTTON_PIN = 17
button = Button(BUTTON_PIN, pull_up=True, hold_time=0.75)


client_proc = None
session_active = [False]  # mutable shared state

timeout_thread = None

def sync_usb_config():
    usb_matches = glob.glob("/media/mjw/TROOPER*/trooper_config.json")
    if usb_matches:
        try:
            shutil.copy(usb_matches[0], "/home/mjw/Trooper/.trooper_config.json")
            print("[Config] USB config copied successfully.")
        except Exception as e:
            print("[Config] Failed to copy USB config:", e)


sync_usb_config()
config = load_config()


def play_message(text):
    voice_model = config.get("voice", "danny-low.onnx")
    device_name = config.get("audio_output_device", "")
    AUDIO_OUTPUT_DEVICE_INDEX = find_device(device_name, is_input=False)
    retro_fx = config.get("retro_voice_fx", False)

    print(f"[Debug] Playing message: '{text}' to device index {AUDIO_OUTPUT_DEVICE_INDEX}")

    # Generate raw PCM from Piper
    proc = subprocess.Popen(
        ["/home/mjw/.local/bin/piper", '--model', f'voices/{voice_model}', '--output_raw'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    raw_pcm, err = proc.communicate(input=text.encode())
    if err:
        print("[Piper Error]", err.decode())

    sample_rate = get_voice_sample_rate(voice_model)    

    # Choose SoX pipeline
    if retro_fx:
        sox_cmd = [
            'sox', '-t', 'raw', '-r', str(sample_rate), '-c', '1', '-b', '16',
            '-e', 'signed-integer', '-', '-r', '48000', '-c', '2', '-t', 'wav', '-',
            'highpass', '300', 'lowpass', '3400',
            'compand', '0.3,1', '6:-70,-60,-20', '-5', '-90', '0.2',
            'gain', '-n', 'vol', '0.9',
            'synth', 'brownnoise', 'mix', '0.01'
        ]
    else:
        sox_cmd = [
            'sox', '-t', 'raw', '-r', str(sample_rate), '-c', '1', '-b', '16',
            '-e', 'signed-integer', '-', '-r', '48000', '-c', '2', '-t', 'wav', '-'
        ]    

    # Pipe PCM through SoX to resample to 48000Hz stereo WAV
    sox = subprocess.Popen(
        sox_cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    wav_bytes, sox_err = sox.communicate(input=raw_pcm)
    if sox_err:
        print("[SoX Error]", sox_err.decode())

    # Playback using PyAudio
    wf = wave.open(io.BytesIO(wav_bytes), 'rb')
    p = pyaudio.PyAudio()
    stream = p.open(
        format=p.get_format_from_width(wf.getsampwidth()),
        channels=wf.getnchannels(),
        rate=wf.getframerate(),
        output=True,
        output_device_index=AUDIO_OUTPUT_DEVICE_INDEX
    )

    data = wf.readframes(1024)
    while data:
        stream.write(data)
        data = wf.readframes(1024)

    stream.stop_stream()
    stream.close()
    p.terminate()
    wf.close()


def session_loop():
    global client_proc
    global config
    greeting_msg = config.get("greeting_message", "").strip()
    timeout_msg = config.get("timeout_message", "").strip()
    timeout_sec = config.get("session_timeout", 0)

    
    # Pre-spin Ollama in the background to make if seem faster for the user's first prompt
    model_name = config.get("model_name")
    if model_name:
        threading.Thread(target=spin_up_ollama, args=(model_name,), daemon=True).start()

    print("[Trooper] Booting up.")
    print("[Trooper] Booting up.")
    if greeting_msg:
        play_message(greeting_msg)

    print("[Debug] Greeting complete, launching client.")

    log_file = open("./client.log", "w")

    client_proc = subprocess.Popen(
        ["python3", "client.py"],
        stdout=log_file,
        stderr=subprocess.STDOUT
    )
    print("[Debug] client.py launched.")

    def monitor_timeout(timeout_sec):
        if timeout_sec <= 0:
            return
        print(f"[Timeout] Session timeout armed for {timeout_sec} seconds.")
        time.sleep(timeout_sec)
        if session_active[0]:
            print("[Timeout] Session timeout expired. Ending session.")
            session_active[0] = False
            end_session(timeout_msg)

    global timeout_thread

    if timeout_thread and timeout_thread.is_alive():
        print("[Debug] Timeout thread already running — skipping.")
    else:
        timeout_thread = threading.Thread(target=monitor_timeout, args=(timeout_sec,), daemon=True)
        timeout_thread.start()

def end_session(msg):
    global client_proc

    if client_proc:
        print("[Trooper] Session ended.")
        client_proc.terminate()
        client_proc.wait()
        client_proc.terminate()
        client_proc.wait()
        if msg:
            play_message(msg)
        time.sleep(1)

def on_button_press():
    global config
    closing_msg = config.get("closing_message", "").strip()
    if not session_active[0]:
        session_active[0] = True
        session_loop()
    else:
        session_active[0] = False
        end_session(closing_msg)

def on_tap():
    print("[Button] Ignored short press")        

def spin_up_ollama(model):
    async def warmup():
        print(f"[Init] Warming up Ollama model: {model}")
        payload = {
            "model": model,
            "messages": [{"role": "system", "content": "System check."}],
            "stream": True
        }
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post("http://localhost:11434/api/chat", json=payload) as resp:
                    async for line in resp.content:
                        if line:
                            print("[Init] Ollama model ready.")
                            break
        except Exception as e:
            print(f"[Init] Ollama warmup failed: {e}")

    asyncio.run(warmup())


def vision_watch_loop():
    serial_mgr = SerialManager()
    print("[Vision] Watching for raised hand (MediaPipe)...")
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("[Vision] Could not open camera.")
        return

    mp_hands = mp.solutions.hands
    hands = mp_hands.Hands(static_image_mode=False, max_num_hands=1, min_detection_confidence=0.6)

    mp_face = mp.solutions.face_detection
    face_detection = mp_face.FaceDetection(model_selection=0, min_detection_confidence=0.5)

    open_streak = 0
    required_streak = 5
    cooldown_seconds = 10
    last_toggle = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        frame = cv2.flip(frame, 1)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb)
        face_results = face_detection.process(rgb)

        # --- Face Recognition (Wake on Gaze) ---
        if face_results.detections:
            # If we see a face, and session is NOT active, wake up?
            # Or just send "Happy" to acknowledge presence?
            
            # 1. Send Happy Eyes (Proactive Friendliness)
            # Throttle this so we don't spam serial
            if now - last_toggle > 5.0 and not session_active[0]:
                 serial_mgr.send("E:HAPPY")
                 # Optional: Start session automatically?
                 # on_button_press() 
                 pass

        if results.multi_hand_landmarks:
            hand = results.multi_hand_landmarks[0]

            fingertips = [
                mp_hands.HandLandmark.THUMB_TIP,
                mp_hands.HandLandmark.INDEX_FINGER_TIP,
                mp_hands.HandLandmark.MIDDLE_FINGER_TIP,
                mp_hands.HandLandmark.RING_FINGER_TIP,
                mp_hands.HandLandmark.PINKY_TIP,
            ]

            up_count = 0
            for tip in fingertips:
                tip_y = hand.landmark[tip].y
                wrist_y = hand.landmark[mp_hands.HandLandmark.WRIST].y
                if tip_y < wrist_y:
                    up_count += 1

            print(f"[Debug] Fingers up: {up_count}, streak: {open_streak}")

            now = time.time()

            if up_count == 5:
                open_streak += 1
            else:
                open_streak = 0

            if open_streak >= required_streak and now - last_toggle > cooldown_seconds:
                print("[Gesture] Open hand detected — toggling session.")
                on_button_press()
                last_toggle = now
                open_streak = 0  # reset after toggle

            # --- Hand Tracking Logic ---
            # Hand X is 0.0 (Left) to 1.0 (Right)
            # Servos are 0 (Right) to 180 (Left) roughly
            # Let's map X to Servo Angle
            # Default Center = 90
            
            wrist_x = hand.landmark[mp_hands.HandLandmark.WRIST].x
            
            # Simple mapping: 0.0 -> 180 (Left), 1.0 -> 0 (Right)
            # Invert because webcam is mirrored usually, let's assume MIRRORED frame
            # If I move hand Right (screen Right), X increases.
            # Robot should look Right (Servo 0).
            
            target_angle = int((1.0 - wrist_x) * 180)
            target_angle = max(0, min(180, target_angle))

            # Send S:L:R (Both eyes/arms move together for head turn effect)
            # But wait, left/right servos might need opposite moves?
            # Assuming simple "Look At" head turn uses same angle or mirrored?
            # Let's assume Head Turn = Left Servo X, Right Servo X?
            # Or Left Servo = Angle, Right Servo = 180-Angle?
            
            # Let's try Parallel movement for now
            serial_mgr.send(f"S:{target_angle}:{target_angle}")


        time.sleep(0.3)

button.when_held = on_button_press
button.when_released = on_tap

print("[System] Awaiting button press...")

if config.get("vision_wake", False):
    threading.Thread(target=vision_watch_loop, daemon=True).start()
    print("[System] Hand-raise wake active.")
else:
    print("[System] Vision wake disabled in config.")

pause()
