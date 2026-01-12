#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "lib/RobotPet.h"
#include "lib/IMUManager.h"
#include "lib/ServoManager.h"
#include "lib/TouchManager.h"
#include "lib/ConfigManager.h"

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define TOUCH_PIN 15

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// I2C Constructor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
IMUManager imu;
ServoManager servo(18, 19); // Left=18, Right=19
TouchManager touch(TOUCH_PIN);
RobotPet robotPet(display, imu, servo, SCREEN_WIDTH, SCREEN_HEIGHT, 100);
ConfigManager configManager;

void scanI2C();
void runDiagnostics();

void setup()
{
  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(50000); // Lower speed for stability
  scanI2C();

  // Diagnostics
  Serial.println("--- STARTUP DIAGNOSTICS ---");
  
  touch.begin();
  
  touch.addClickCallback([](int count) { 
      // robotPet.shortClick(count); // Optional if touch-down handles immediate reaction
      // But keeping proper click handling is good for transitions if needed
      if (count > 0) robotPet.shortClick(count);
  });

  touch.addLongPressCallback([]() {
      robotPet.longClick();
  });

  touch.addLongPressReleaseCallback([]() { 
      robotPet.longClickRelease(); 
  });

  // Initialize I2C Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
      Serial.println("[Display] I2C Allocation Failed");
  } else {
      Serial.println("[Display] I2C Initialized");
  }

  robotPet.begin();
  display.display();

  runDiagnostics();
  Serial.println("--- END DIAGNOSTICS ---");

  robotPet.start();
}

void loop()
{
  touch.update();
  robotPet.update();
}

void scanI2C()
{
  Serial.println("[I2C] Scanning...");
  for (byte addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.print("[I2C] Found: 0x");
      Serial.println(addr, HEX);
    }
  }
}

void runDiagnostics() {
    Serial.println("[DIAG] Testing Servos (Wiggle)...");
    servo.startWiggle();
    unsigned long start = millis();
    while(millis() - start < 1000) {
        servo.update();
        delay(10);
    }
    servo.stop();
    Serial.println("[DIAG] Servo test complete.");

    Serial.println("[DIAG] checking Touch...");
    int touchVal = digitalRead(TOUCH_PIN);
    Serial.printf("[DIAG] Touch Pin (%d) Value: %d\n", TOUCH_PIN, touchVal);

    Serial.println("[DIAG] Checking I2C Devices Again...");
    scanI2C();

    Serial.println("[DIAG] Drawing Test Pattern...");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("DIAGNOSTIC");
    display.println("TEST MODE");
    display.drawRect(10, 20, 50, 20, SSD1306_WHITE);
    display.display();
    delay(2000); // Keep it on screen for 2 seconds
}