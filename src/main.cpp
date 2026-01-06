#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "lib/SoundPlayer.h"
#include "lib/RobotPet.h"
#include "lib/MotorManager.h"
#include "lib/ButtonManager.h"
#include "lib/BLEManager.h"
#include "lib/MediaVisualizer.h"
#include "lib/NotificationManager.h"
#include "lib/MenuManager.h"
#include "lib/ConfigManager.h"
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

#define BUZZER_PIN 8
#define BUTTON_PIN 10

#define MOTOR_IN1 0
#define MOTOR_IN2 1
#define MOTOR_IN3 2
#define MOTOR_IN4 3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MotorManager motor(MOTOR_IN1, MOTOR_IN2, MOTOR_IN3, MOTOR_IN4);
SoundPlayer melody(BUZZER_PIN);
ButtonManager button(BUTTON_PIN);
BLEManager ble;
RobotPet robotPet(display, melody, motor, SCREEN_WIDTH, SCREEN_HEIGHT, 100);
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

void setup()
{
  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  scanI2C();

  SettingConfig settingConfig = configManager.loadSettingsConfig();

  bluetoothEnabled = settingConfig.bluetooth;
  wifiEnabled = settingConfig.wifi;

  button.begin();
  button.addClickCallback([](int count)
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

  button.addLongPressCallback([]()
                              {
    if (currentState == Menu)
    {
      menu.selectItem();
    }
    else if (currentState == Animation)
    {
      robotPet.longClick();
    } });

  button.addLongPressReleaseCallback([]()
                                     { 
    if (currentState == Animation) robotPet.longClickRelease(); });

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println("[Display] Failed!");
    while (1)
      ;
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
    Serial.println("[BLE] Connected");
 
      melody.play("G4 100 20 C5 100 20 E5 100 20 G5 100 20 C6 100 20 D6 100 20 E6 200 200"); });
  ble.setOnDisconnectCallback([]()
                              {
    Serial.println("[BLE] Disconnected");

      melody.play("E6 100 20 D6 100 20 C6 120 40 G5 150 100");
    
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

  display.clearDisplay();
  display.display();

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
      melody.play("C5 100 20 E5 100 20 G5 150 20");
      configManager.saveSettingsConfig("bluetooth", true);
    } else {
      ble.turnOff();
      melody.play("G5 100 20 E5 100 20 C5 150 20");
      configManager.saveSettingsConfig("bluetooth", false);
    } });

  menu.addActionToSubmenu(bluetoothMenu, "Reconnect", []()
                          {
    Serial.println("[BLE] Reconnecting...");
    ble.turnOff();
    delay(500);
    ble.turnOn();
   melody.play("C5 100 20 G5 100 20"); });

  menu.addInfoToSubmenu(bluetoothMenu, "Status", []()
                        { return bluetoothEnabled ? "Active" : "Off"; });

  auto wifiMenu = menu.createSubmenu();
  menu.addToggleToSubmenu(wifiMenu, "WiFi Enable", &wifiEnabled, [](bool state)
                          {
    Serial.print("[WiFi] Turned ");
    Serial.println(state ? "ON" : "OFF");
   
      if (state) {
        melody.play("C5 100 20 E5 100 20");
      } else {
        melody.play("E5 100 20 C5 100 20");
      } });

  menu.addActionToSubmenu(wifiMenu, "Scan Networks", []()
                          {
    Serial.println("[WiFi] Scanning...");
     melody.play("C5 50 10 E5 50 10 G5 50 10"); });

  menu.addInfoToSubmenu(wifiMenu, "Status", []()
                        { return wifiEnabled ? "Connected" : "Off"; });

  menu.addSubmenuToSubmenu(connectivityMenu, "Bluetooth", bluetoothMenu);
  menu.addSubmenuToSubmenu(connectivityMenu, "WiFi", wifiMenu);

  menu.addSubmenu("Connectivity", connectivityMenu);

  menu.addItem("Exit", ACTION, []()
               {
    Serial.println("[Menu] Exiting...");
    melody.play("G5 100 20 E5 100 20 C5 150 20");
    menu.hide();
    switchState(Animation); });
}

void loop()
{
  button.update();
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

    melody.play("C6 120 40 E6 120 40 G6 200 100");
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