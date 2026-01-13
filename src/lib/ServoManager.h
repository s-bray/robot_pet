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
  
  int wiggleSpeed; 

public:
  ServoManager(int leftPin, int rightPin);
  void setWiggleSpeed(int ms);
  void begin();
  void update();
  void startWiggle();
  void stop();
  void move(int leftAngle, int rightAngle);
};

#endif
