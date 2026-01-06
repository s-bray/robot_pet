#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>

class BLEManager;

class MyBLEServerCallbacks : public BLEServerCallbacks
{
private:
  BLEManager *manager;

public:
  MyBLEServerCallbacks(BLEManager *mgr);
  void onConnect(BLEServer *pServer);
  void onDisconnect(BLEServer *pServer);
};

class MyBLECharacteristicCallbacks : public BLECharacteristicCallbacks
{
private:
  BLEManager *manager;

public:
  MyBLECharacteristicCallbacks(BLEManager *mgr);
  void onWrite(BLECharacteristic *pCharacteristic);
};

class BLEManager
{
private:
  BLEServer *pServer;
  BLEService *pService;
  BLECharacteristic *pCharacteristic;
  bool deviceConnected;
  bool bleEnabled;
  MyBLEServerCallbacks *pCallbacks;
  MyBLECharacteristicCallbacks *pCharCallbacks;

  std::function<void(String)> onMessageCallback;
  std::function<void()> onConnectCallback;
  std::function<void()> onDisconnectCallback;

  friend class MyBLEServerCallbacks;
  friend class MyBLECharacteristicCallbacks;

public:
  BLEManager();
  void begin(const char *deviceName);
  bool isConnected();
  void sendData(String data);

  void turnOn();
  void turnOff();

  bool isEnabled();

  void setOnMessageCallback(std::function<void(String)> callback);
  void setOnConnectCallback(std::function<void()> callback);
  void setOnDisconnectCallback(std::function<void()> callback);

  void setDeviceConnected(bool connected);
  void handleMessage(String message);
};

#endif