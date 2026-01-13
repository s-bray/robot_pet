#include "ServoManager.h"

ServoManager::ServoManager(int leftPin, int rightPin)
    : pinLeft(leftPin), pinRight(rightPin), isWiggling(false), lastUpdate(0), wiggleStep(0), wiggleSpeed(200) {}

void ServoManager::setWiggleSpeed(int ms) {
    if (ms > 0) wiggleSpeed = ms;
}

void ServoManager::begin()
{
  // ... (rest of begin)
  // Allocate timers for ESP32 servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  servoLeft.setPeriodHertz(50);
  servoRight.setPeriodHertz(50);
}

void ServoManager::update()
{
  if (!isWiggling) return;

  unsigned long now = millis();
  if (now - lastUpdate >= wiggleSpeed) {
    lastUpdate = now;
    wiggleStep++;
    
    // Simple 4-step wiggle pattern
    // 0: Left=High, Right=Low
    // 1: Center
    // 2: Left=Low, Right=High
    // 3: Center
    
    int phase = wiggleStep % 4;
    
    if (phase == 0) {
      servoLeft.write(CENTER_POS + WIGGLE_RANGE);
      servoRight.write(CENTER_POS - WIGGLE_RANGE);
    } else if (phase == 2) {
      servoLeft.write(CENTER_POS - WIGGLE_RANGE);
      servoRight.write(CENTER_POS + WIGGLE_RANGE);
    } else {
      servoLeft.write(CENTER_POS);
      servoRight.write(CENTER_POS);
    }
  }
}

void ServoManager::startWiggle()
{
  if (isWiggling) return;
  
  if (!servoLeft.attached()) servoLeft.attach(pinLeft, 500, 2400);
  if (!servoRight.attached()) servoRight.attach(pinRight, 500, 2400);
  
  isWiggling = true;
  wiggleStep = 0;
  lastUpdate = 0; // Trigger immediate update
}

void ServoManager::stop()
{
  if (!isWiggling) return;
  
  isWiggling = false;
  
  // Return to center
  if (servoLeft.attached()) servoLeft.write(CENTER_POS);
  if (servoRight.attached()) servoRight.write(CENTER_POS);
  
  delay(200); // Give time to reach center
  
  servoLeft.detach();
  servoRight.detach();
}

void ServoManager::move(int leftAngle, int rightAngle)
{
  if (!servoLeft.attached()) servoLeft.attach(pinLeft, 500, 2400);
  if (!servoRight.attached()) servoRight.attach(pinRight, 500, 2400);

  isWiggling = false; // Manual move cancels wiggle
  
  servoLeft.write(constrain(leftAngle, 0, 180));
  servoRight.write(constrain(rightAngle, 0, 180));
}
