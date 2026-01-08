#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ServoManager
{
private:
  Servo servoLeft;
  Servo servoRight;
  int pinLeft;
  int pinRight;

  bool isWiggling;
  unsigned long lastUpdate;
  int wiggleStep;
  
  static const int CENTER_POS = 90;
  static const int WIGGLE_RANGE = 30; // +/- degrees
  static const int WIGGLE_SPEED = 100; // ms per step

public:
  ServoManager(int leftPin, int rightPin);
  void begin();
  void update();
  void startWiggle();
  void stop();
};

#endif
