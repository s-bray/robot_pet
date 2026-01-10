#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "lib/RobotPet.h"
#include "lib/IMUManager.h"
#include "lib/ServoManager.h"
#include "lib/TouchManager.h"
#include "lib/BLEManager.h"
#include "lib/MediaVisualizer.h"
#include "lib/NotificationManager.h"
#include "lib/MenuManager.h"
#include "lib/ConfigManager.h"
#include <ArduinoJson.h>

// SPI OLED DEFINITIONS
#define OLED_MOSI   33 // SDA
#define OLED_CLK    32 // SCK
#define OLED_DC     26
#define OLED_CS     27
#define OLED_RESET  25

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 // Confirmed 32px height
//#define SCREEN_ADDRESS 0x3C // Not used for SPI

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

#define TOUCH_PIN 15

// Software SPI Constructor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
IMUManager imu;
ServoManager servo(18, 19); // Left=18, Right=19
TouchManager touch(TOUCH_PIN);
BLEManager ble;
RobotPet robotPet(display, imu, servo, SCREEN_WIDTH, SCREEN_HEIGHT, 100);
MediaVisualizer visualizer(display, FPS_30);
NotificationManager notification(display, 15000);
MenuManager menu(display);
ConfigManager configManager;

enum CurrentState
{
  Animation,
  Media,
  Notification,
  Menu
};
CurrentState currentState = Animation;
CurrentState previousState = Animation;

unsigned long lastMediaActive = 0;
static const unsigned long MEDIA_TIMEOUT = 5000;
static const float AUDIO_THRESHOLD = 0.01f;

bool bluetoothEnabled = false;
bool wifiEnabled = false;
String firmwareVersion = "v1.0.0";

void handleBLEMessage(String message);
void scanI2C();
void switchState(CurrentState newState);
void updateCurrentState();
void setupMenu();
void runDiagnostics();

void setup()
{
  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(50000); // Lower speed for stability
  scanI2C();

  SettingConfig settingConfig = configManager.loadSettingsConfig();

  bluetoothEnabled = settingConfig.bluetooth;
  wifiEnabled = settingConfig.wifi;

  // Diagnostics moved to end of setup


  touch.begin();
  touch.addClickCallback([](int count)
                          { 
    if (currentState == Menu) {
      if (count == 1) {
        menu.navigateDown();
      } else if (count == 2) {
        menu.navigateUp();
      } else if (count == 3) {
        menu.back(); 
      }
    } else if (currentState == Animation) {
      if (count == 5) {
        switchState(Menu);
      } else {
        robotPet.shortClick(count);
      }
    } });

  touch.addLongPressCallback([]()
                              {
    if (currentState == Menu)
    {
      menu.selectItem();
    }
    else if (currentState == Animation)
    {
      robotPet.longClick();
    } });

  touch.addLongPressReleaseCallback([]()
                                     { 
    if (currentState == Animation) robotPet.longClickRelease(); });

  // Initialize SPI Display
  // Note: SPI display doesn't have an address like 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
      Serial.println("[Display] SPI Allocation Failed");
  } else {
      Serial.println("[Display] SPI Initialized");
  }

  robotPet.begin();
  visualizer.begin();
  notification.begin();
  menu.begin();
  setupMenu();

  ble.setOnMessageCallback([](String message)
                           { handleBLEMessage(message); });
  ble.setOnConnectCallback([]()
                           { 
    Serial.println("[BLE] Connected"); });

  ble.setOnDisconnectCallback([]()
                              {
    Serial.println("[BLE] Disconnected");
    
    if (currentState != Menu) {
      switchState(Animation);
    } });

  ble.begin("PetRobot-c3");

  if (bluetoothEnabled)
  {
    ble.turnOn();
  }
  else
  {
    ble.turnOff();
  }

  display.display();

  Serial.println("--- STARTUP DIAGNOSTICS ---");
  runDiagnostics();
  Serial.println("--- END DIAGNOSTICS ---");

  robotPet.start();
}

void setupMenu()
{
  menu.setMenuTitle("Main Menu");

  auto connectivityMenu = menu.createSubmenu();

  auto bluetoothMenu = menu.createSubmenu();
  menu.addToggleToSubmenu(bluetoothMenu, "BT Enable", &bluetoothEnabled, [](bool state)
                          {
    Serial.print("[BLE] Turned ");
    Serial.println(state ? "ON" : "OFF");
    if (state) {
      ble.turnOn();
      
      configManager.saveSettingsConfig("bluetooth", true);
    } else {
      ble.turnOff();
      
      configManager.saveSettingsConfig("bluetooth", false);
    } });

  menu.addActionToSubmenu(bluetoothMenu, "Reconnect", []()
                          {
    Serial.println("[BLE] Reconnecting...");
    ble.turnOff();
    delay(500);
    ble.turnOn();
    
 });

  menu.addInfoToSubmenu(bluetoothMenu, "Status", []()
                        { return bluetoothEnabled ? "Active" : "Off"; });

  auto wifiMenu = menu.createSubmenu();
  menu.addToggleToSubmenu(wifiMenu, "WiFi Enable", &wifiEnabled, [](bool state)
                          {
    Serial.print("[WiFi] Turned ");
    Serial.println(state ? "ON" : "OFF");
  });

  menu.addActionToSubmenu(wifiMenu, "Scan Networks", []()
                          {
    Serial.println("[WiFi] Scanning...");
 });

  menu.addInfoToSubmenu(wifiMenu, "Status", []()
                        { return wifiEnabled ? "Connected" : "Off"; });

  menu.addSubmenuToSubmenu(connectivityMenu, "Bluetooth", bluetoothMenu);
  menu.addSubmenuToSubmenu(connectivityMenu, "WiFi", wifiMenu);

  menu.addSubmenu("Connectivity", connectivityMenu);

  menu.addItem("Exit", ACTION, []()
               {
    Serial.println("[Menu] Exiting...");
    menu.hide();
    switchState(Animation); });
}

void loop()
{
  touch.update();
  updateCurrentState();
}

void switchState(CurrentState newState)
{
  if (currentState == newState)
    return;

  if (newState != Notification && newState != Menu)
  {
    previousState = currentState;
  }

  if (currentState == Animation)
  {
    robotPet.stop();
  }
  else if (currentState == Media)
  {
    visualizer.stop();
  }
  else if (currentState == Menu)
  {
    menu.hide();
  }

  Serial.print("[State] ");
  Serial.print(currentState == Animation ? "Animation" : currentState == Media ? "Media"
                                                     : currentState == Menu    ? "Menu"
                                                                               : "Notification");
  Serial.print(" -> ");
  Serial.println(newState == Animation ? "Animation" : newState == Media ? "Media"
                                                   : newState == Menu    ? "Menu"
                                                                         : "Notification");

  currentState = newState;

  if (currentState == Animation)
  {
    robotPet.start();
  }
  else if (currentState == Media)
  {
    lastMediaActive = millis();
  }
  else if (currentState == Menu)
  {
    menu.show();
  }
}

void updateCurrentState()
{
  unsigned long now = millis();
  unsigned long elapsedMediaActive =
      (now >= lastMediaActive) ? (now - lastMediaActive) : 0;

  switch (currentState)
  {
  case Animation:
    robotPet.update();
    break;

  case Media:
    visualizer.update();

    if (elapsedMediaActive > MEDIA_TIMEOUT)
    {
      Serial.println("[Media] Timeout - no audio detected");
      switchState(Animation);
    }
    break;

  case Notification:
    notification.update();

    if (notification.isExpired())
    {
      Serial.println("[Notification] Expired");
      switchState(previousState);
    }
    break;

  case Menu:
    menu.update();
    break;
  }
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

void handleBLEMessage(String message)
{
  if (message.startsWith("\"") && message.endsWith("\""))
  {
    message = message.substring(1, message.length() - 1);
  }
  message.replace("\\\"", "\"");

  JsonDocument doc;
  if (deserializeJson(doc, message))
  {
    Serial.println("[JSON] Parse error");
    return;
  }

  const char *type = doc["type"] | "";

  if (strcmp(type, "notification") == 0)
  {
    if (currentState != Notification && currentState != Menu)
    {
      previousState = currentState;
    }

    if (currentState != Menu)
    {
      switchState(Notification);
    }

    notification.show(doc);
    return;
  }

  if (strcmp(type, "media") == 0)
  {
    if (currentState == Notification || currentState == Menu)
    {
      if (currentState != Menu)
      {
        previousState = Media;
      }
      return;
    }

    if (doc["audio_amplitude"].is<JsonObject>())
    {
      float amplitude = doc["audio_amplitude"]["amplitude"] | 0.0f;

      if (amplitude > AUDIO_THRESHOLD)
      {
        if (currentState != Media)
        {
          switchState(Media);
        }

        visualizer.handleMediaData(doc);
        lastMediaActive = millis();
      }
    }

    return;
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