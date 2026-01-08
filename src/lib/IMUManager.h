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

  static constexpr float SHAKE_THRESHOLD = 8.0f; // rad/s (approx)
  static constexpr float PICKUP_THRESHOLD = 1.0f; // Normalized acceleration variance

  // Activity average tracking
  static const int SAMPLE_SIZE = 10;
  float gyroHistory[SAMPLE_SIZE];
  float accelHistory[SAMPLE_SIZE];
  int historyIndex = 0;

  bool isConnected = false;

public:
  void begin();
  void update();
  
  bool isShaken();
  bool isPickedUp();
};

#endif
