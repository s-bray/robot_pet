import cv2
import threading
import time
import os
from utils import SerialManager

class VisionSystem:
    def __init__(self, serial_mgr=None):
        self.running = False
        self.serial_mgr = serial_mgr or SerialManager()
        self.cap = None
        self.face_cascade = None
        
        # Load Haar Cascade
        # We'll use the one included with cv2 or download/locate it?
        # Usually checking cv2.data.haarcascades path
        cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
        if not os.path.exists(cascade_path):
             # Fallback simple path if data attribute fails
             cascade_path = '/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml'
             
        self.face_cascade = cv2.CascadeClassifier(cascade_path)
        if self.face_cascade.empty():
            print(f"[Vision] Error: Could not load Haar Cascade from {cascade_path}")
            
    def start(self):
        if self.running: return
        self.running = True
        threading.Thread(target=self._loop, daemon=True).start()
        print("[Vision] System started.")

    def stop(self):
        self.running = False
        if self.cap:
            self.cap.release()

    def _loop(self):
        self.cap = cv2.VideoCapture(0)
        # Lower resolution for performance on Pi
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
        
        if not self.cap.isOpened():
             print("[Vision] Failed to open camera.")
             return

        last_move_time = 0
        last_face_time = 0
        
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                time.sleep(0.1)
                continue

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = self.face_cascade.detectMultiScale(gray, 1.1, 4)

            now = time.time()
            if len(faces) > 0:
                # Greeting Logic: If we haven't seen a face for a while, smile!
                if now - last_face_time > 5.0:
                    self.serial_mgr.send("E:HAPPY")
                    # Optional: We could trigger a voice greeting here if we had access to the pipeline
                
                last_face_time = now

                # Find largest face
                largest_face = max(faces, key=lambda r: r[2] * r[3])
                (x, y, w, h) = largest_face
                
                # Calculate center X (0 to 1 range relative to frame width)
                frame_width = frame.shape[1]
                center_x = x + w / 2
                
                # Normalize 0.0 to 1.0
                norm_x = center_x / frame_width
                
                # Invert because camera is mirrored? 
                # If I move Right (screen right), norm_x increases. 
                # I want robot to look Right (Servo 0)
                # So 1.0 -> 0, 0.0 -> 180
                
                target_angle = int((1.0 - norm_x) * 180)
                target_angle = max(0, min(180, target_angle))
                
                # Throttle serial commands
                now = time.time()
                if now - last_move_time > 0.1: # 100ms
                    self.serial_mgr.send(f"S:{target_angle}:{target_angle}")
                    last_move_time = now
                    print(f"[Vision] Face at {norm_x:.2f}, Moving to {target_angle}")

            time.sleep(0.05) # ~20 FPS cap

        self.cap.release()
        print("[Vision] System stopped.")
