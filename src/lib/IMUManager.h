#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

class IMUManager
{
private:
  Adafruit_MPU6050 mpu;
  
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  float totalAccel;
  float totalGyro;

  static constexpr float SHAKE_THRESHOLD = 5.0f; // Lowered from 8.0 for easier detection
  static constexpr float PICKUP_THRESHOLD = 2.0f; // Increased to reduce false positives

  // Activity average tracking
  static const int SAMPLE_SIZE = 10;
  float gyroHistory[SAMPLE_SIZE];
  float accelHistory[SAMPLE_SIZE];
  int historyIndex = 0;
  
  unsigned long lastUpdate = 0;
  static const int UPDATE_INTERVAL = 20; // Increased rate for better shake detection

  bool isConnected = false;

public:
  void begin();
  void update();
  
  bool isShaken();
  bool isPickedUp();
};

#endif
