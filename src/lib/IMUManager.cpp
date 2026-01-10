#include "IMUManager.h"

void IMUManager::begin()
{
  Serial.println("[IMU] Initializing MPU6050...");
  
  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("[IMU] Failed to find MPU6050 chip");
    isConnected = false;
    return;
  }
  
  Serial.println("[IMU] MPU6050 Found!");
  isConnected = true;

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize arrays
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    gyroHistory[i] = 0;
    accelHistory[i] = 0;
  }
}

void IMUManager::update()
{
  if (!isConnected) return;

  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_INTERVAL) return;
  lastUpdate = now;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  accelX = a.acceleration.x;
  accelY = a.acceleration.y;
  accelZ = a.acceleration.z;
  
  gyroX = g.gyro.x;
  gyroY = g.gyro.y;
  gyroZ = g.gyro.z;

  // Calculate magnitudes
  totalAccel = sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  totalGyro = sqrt(gyroX*gyroX + gyroY*gyroY + gyroZ*gyroZ);

  // Update history
  gyroHistory[historyIndex] = totalGyro;
  accelHistory[historyIndex] = totalAccel;
  historyIndex = (historyIndex + 1) % SAMPLE_SIZE;
}

bool IMUManager::isShaken()
{
  if (!isConnected) return false;
  
  // Calculate average gyro over recent history
  float avgGyro = 0;
  for (int i=0; i<SAMPLE_SIZE; i++) avgGyro += gyroHistory[i];
  avgGyro /= SAMPLE_SIZE;

  // If average rotation is very high, it's being shaken
  return avgGyro > SHAKE_THRESHOLD;
}

bool IMUManager::isPickedUp()
{
  if (!isConnected) return false;

  // Simple heuristic: if Z acceleration drops significantly (lifting up fast)
  // or if there is moderate continuous movement without violent shaking
  
  float avgAccel = 0;
  for (int i=0; i<SAMPLE_SIZE; i++) avgAccel += accelHistory[i];
  avgAccel /= SAMPLE_SIZE;

  // Normal gravity is ~9.8 m/s^2. 
  // If we detect significant deviation from gravity BUT not violent rotation (shaking)
  // that implies being moved/lifted.
  
  float accelVariance = abs(avgAccel - 9.8);
  float avgGyro = 0;
  for (int i=0; i<SAMPLE_SIZE; i++) avgGyro += gyroHistory[i];
  avgGyro /= SAMPLE_SIZE;

  // It is picked up if it's moving (Variance > Threshold) but NOT being shaken violently
  return (accelVariance > PICKUP_THRESHOLD && avgGyro < (SHAKE_THRESHOLD / 2.0));
}
