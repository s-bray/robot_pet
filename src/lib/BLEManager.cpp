#include "BLEManager.h"

MyBLEServerCallbacks::MyBLEServerCallbacks(BLEManager *mgr) : manager(mgr)
{
}

void MyBLEServerCallbacks::onConnect(BLEServer *pServer)
{
  manager->setDeviceConnected(true);
  if (manager->onConnectCallback)
  {
    manager->onConnectCallback();
  }
}

void MyBLEServerCallbacks::onDisconnect(BLEServer *pServer)
{
  manager->setDeviceConnected(false);
  if (manager->onDisconnectCallback)
  {
    manager->onDisconnectCallback();
  }
  // Hanya restart advertising jika BLE masih enabled
  if (manager->isEnabled())
  {
    pServer->startAdvertising();
  }
}

MyBLECharacteristicCallbacks::MyBLECharacteristicCallbacks(BLEManager *mgr) : manager(mgr)
{
}

void MyBLECharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string value = pCharacteristic->getValue();
  if (value.length() > 0)
  {
    String message = String(value.c_str());
    manager->handleMessage(message);
  }
}

BLEManager::BLEManager()
{
  pServer = nullptr;
  pService = nullptr;
  pCharacteristic = nullptr;
  deviceConnected = false;
  bleEnabled = false;
  pCallbacks = nullptr;
  pCharCallbacks = nullptr;
  onMessageCallback = nullptr;
  onConnectCallback = nullptr;
  onDisconnectCallback = nullptr;
}

void BLEManager::begin(const char *deviceName)
{
  BLEDevice::init(deviceName);
  BLEDevice::setMTU(517);

  pServer = BLEDevice::createServer();
  pCallbacks = new MyBLEServerCallbacks(this);
  pServer->setCallbacks(pCallbacks);

  BLEUUID serviceUUID("1b5dccd3-ef4d-4df0-94b2-7f411f1e0844");
  pService = pServer->createService(serviceUUID);

  BLEUUID charUUID("1999eae0-d5ad-4909-aff3-4a8875149db5");
  pCharacteristic = pService->createCharacteristic(
      charUUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  pCharCallbacks = new MyBLECharacteristicCallbacks(this);
  pCharacteristic->setCallbacks(pCharCallbacks);

  BLE2902 *pDescriptor = new BLE2902();
  pCharacteristic->addDescriptor(pDescriptor);

  pCharacteristic->setValue("Ready");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(serviceUUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  bleEnabled = true; // BLE aktif setelah begin()
}

void BLEManager::turnOn()
{
  if (!bleEnabled)
  {
    bleEnabled = true;

    // Mulai advertising untuk menerima koneksi baru
    if (pServer != nullptr)
    {
      BLEDevice::startAdvertising();
      Serial.println("BLE: Advertising started");
    }
  }
}

void BLEManager::turnOff()
{
  if (bleEnabled)
  {
    bleEnabled = false;

    // Putuskan koneksi yang ada
    if (deviceConnected && pServer != nullptr)
    {
      pServer->disconnect(pServer->getConnId());
      Serial.println("BLE: Connection disconnected");
    }

    // Hentikan advertising
    BLEDevice::getAdvertising()->stop();
    Serial.println("BLE: Advertising stopped");
  }
}

bool BLEManager::isEnabled()
{
  return bleEnabled;
}

bool BLEManager::isConnected()
{
  return deviceConnected;
}

void BLEManager::sendData(String data)
{
  if (deviceConnected && pCharacteristic != nullptr && bleEnabled)
  {
    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();
  }
}

void BLEManager::setDeviceConnected(bool connected)
{
  deviceConnected = connected;
}

void BLEManager::handleMessage(String message)
{
  if (onMessageCallback)
  {
    onMessageCallback(message);
  }
}

void BLEManager::setOnMessageCallback(std::function<void(String)> callback)
{
  onMessageCallback = callback;
}

void BLEManager::setOnConnectCallback(std::function<void()> callback)
{
  onConnectCallback = callback;
}

void BLEManager::setOnDisconnectCallback(std::function<void()> callback)
{
  onDisconnectCallback = callback;
}