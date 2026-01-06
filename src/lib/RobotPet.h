#ifndef ROBOT_PET_H
#define ROBOT_PET_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <lib/FluxGarage_RoboEyes.h>
#include "SoundPlayer.h"
#include "MotorManager.h"

class RobotPet
{
private:
  Adafruit_SSD1306 &display;
  RoboEyes<Adafruit_SSD1306> roboEyes;
  SoundPlayer &melody;
  MotorManager &motor;

  int screenWidth, screenHeight, refreshDelay;
  bool isRunning;

  enum EyeState
  {
    Default,
    Happy,
    LongHappy,
    Scared,
    Scare,
    Curiosity,
    Sleepy,
    Asleep,
    Angry
  } currentEyeState;

  static const unsigned long IDLE_DELAY = 5000;
  static const unsigned long DEEP_SLEEP_DELAY = 10000;
  static const unsigned long ANGRY_DURATION = 5000;
  static const unsigned long HAPPY_DURATION = 500;
  static const unsigned long CURIOSITY_DURATION = 10000;
  static const unsigned long SCARE_DURATION = 4000;
  static const unsigned long SCARED_DURATION = 2000;

  unsigned long lastActionTime;
  unsigned long lastMotorActionTime;
  unsigned long randomMotorInterval;

  // Array untuk menyimpan riwayat gerakan
  static const int MAX_MOVEMENT_HISTORY = 6;
  int movementHistory[MAX_MOVEMENT_HISTORY];
  int movementCount;
  int leftCount;
  int rightCount;

  void setDefaultState()
  {
    roboEyes.setMood(DEFAULT);
    roboEyes.setWidth(30, 30);
    roboEyes.setHeight(36, 36);
    roboEyes.setSpacebetween(16);
    roboEyes.setBorderradius(6, 6);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setAutoblinker(ON, 2, 2);
    roboEyes.setIdleMode(OFF);
    roboEyes.setHFlicker(OFF);
    roboEyes.setVFlicker(OFF);
    roboEyes.setSweat(OFF);
    roboEyes.setCuriosity(OFF);
  }

  void enterDefaultState()
  {
    currentEyeState = Default;
    setDefaultState();
  }

  void enterHappyState()
  {
    currentEyeState = Happy;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.anim_laugh();
    roboEyes.setIdleMode(OFF);
    roboEyes.setAutoblinker(OFF);
    Serial.println("CurrentState: Happy");
  }

  void enterLongHappyState()
  {
    currentEyeState = LongHappy;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setVFlicker(ON, 5);
    roboEyes.setIdleMode(OFF);
    roboEyes.setAutoblinker(OFF);
    Serial.println("CurrentState: LongHappy");
  }

  void enterScaredState()
  {
    currentEyeState = Scared;
    roboEyes.setMood(TIRED);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setSweat(ON);
    roboEyes.setVFlicker(ON, 3);
    roboEyes.setHFlicker(ON, 3);
    roboEyes.setAutoblinker(OFF);
    Serial.println("CurrentState: Scared");
  }

  void enterScareState()
  {
    currentEyeState = Scare;
    roboEyes.setMood(TIRED);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setSweat(OFF);
    roboEyes.setVFlicker(ON, 3);
    roboEyes.setHFlicker(ON, 3);
    roboEyes.setAutoblinker(OFF);
    Serial.println("CurrentState: Scare");
  }

  void enterCuriosityState()
  {
    currentEyeState = Curiosity;
    roboEyes.setMood(DEFAULT);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setAutoblinker(ON, 2, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    roboEyes.setHFlicker(OFF);
    roboEyes.setSweat(OFF);
    roboEyes.setCuriosity(ON);
    Serial.println("CurrentState: Curiosity");
  }

  void enterSleepyState()
  {
    currentEyeState = Sleepy;
    roboEyes.setMood(TIRED);
    roboEyes.setHeight(20, 20);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setSweat(OFF);
    roboEyes.setAutoblinker(ON, 2, 2);
    roboEyes.setIdleMode(OFF);
    Serial.println("CurrentState: Sleepy");
  }

  void enterAsleepState()
  {
    currentEyeState = Asleep;
    roboEyes.setMood(DEFAULT);
    roboEyes.setPosition(SOUTH);
    roboEyes.setHeight(3, 3);
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);
    roboEyes.setBorderradius(0, 0);
    Serial.println("CurrentState: Asleep");
  }

  void enterAngryState()
  {
    currentEyeState = Angry;
    roboEyes.setMood(ANGRY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setHFlicker(ON, 2);
    Serial.println("CurrentState: Angry");
  }

  void resetMovementHistory()
  {
    movementCount = 0;
    leftCount = 0;
    rightCount = 0;
    for (int i = 0; i < MAX_MOVEMENT_HISTORY; i++)
    {
      movementHistory[i] = 0;
    }
    Serial.println("Movement history reset");
  }

  void addMovementToHistory(int movement)
  {
    movementHistory[movementCount] = movement;

    if (movement == 3) // Left
    {
      leftCount++;
    }
    else if (movement == 4) // Right
    {
      rightCount++;
    }

    movementCount++;

    // Print history untuk debugging
    Serial.print("Movement history [");
    Serial.print(movementCount);
    Serial.print("/12]: ");
    for (int i = 0; i < movementCount; i++)
    {
      if (movementHistory[i] == 1)
        Serial.print("F");
      else if (movementHistory[i] == 2)
        Serial.print("B");
      else if (movementHistory[i] == 3)
        Serial.print("L");
      else if (movementHistory[i] == 4)
        Serial.print("R");
      Serial.print(" ");
    }
    Serial.print(" | L:");
    Serial.print(leftCount);
    Serial.print(" R:");
    Serial.println(rightCount);
  }

  bool canResetMovementHistory()
  {
    // Cek apakah sudah mencapai 12 gerakan DAN left/right seimbang
    return (movementCount >= MAX_MOVEMENT_HISTORY && leftCount == rightCount);
  }

  void randomMotorMovement()
  {
    if (!isRunning)
    {
      motor.stop();
      return;
    }

    // Cek apakah perlu reset
    if (canResetMovementHistory())
    {
      Serial.println(">>> Balanced! Resetting history <<<");
      resetMovementHistory();
    }

    // Tentukan pilihan gerakan berdasarkan kondisi left/right
    unsigned int motorChoice;

    if (movementCount >= MAX_MOVEMENT_HISTORY)
    {
      // Sudah 12 gerakan tapi belum seimbang
      // Paksa pilih gerakan yang kurang
      if (leftCount < rightCount)
      {
        motorChoice = 3; // Left
        Serial.println("Force LEFT to balance");
      }
      else if (rightCount < leftCount)
      {
        motorChoice = 4; // Right
        Serial.println("Force RIGHT to balance");
      }
      else
      {
        // Seharusnya tidak sampai sini karena sudah di-reset di atas
        motorChoice = random(1, 5);
      }
    }
    else
    {
      // Belum 12 gerakan, pilih random
      motorChoice = random(1, 5);
    }

    // Eksekusi gerakan
    switch (motorChoice)
    {
    case 1:
      Serial.print("Motor: Forward");
      motor.forward();
      break;
    case 2:
      Serial.print("Motor: Backward");
      motor.backward();
      break;
    case 3:
      Serial.print("Motor: Left");
      motor.left();
      break;
    case 4:
      Serial.print("Motor: Right");
      motor.right();
      break;
    }

    // Simpan ke history
    addMovementToHistory(motorChoice);

    delay(75);
    motor.stop();
  }

public:
  RobotPet(Adafruit_SSD1306 &disp, SoundPlayer &buzzer, MotorManager &mtr, int width, int heigh, int delay)
      : display(disp), roboEyes(disp), melody(buzzer), motor(mtr),
        screenWidth(width), screenHeight(heigh), refreshDelay(delay),
        currentEyeState(Default), lastActionTime(0), lastMotorActionTime(0),
        randomMotorInterval(0), isRunning(false), movementCount(0),
        leftCount(0), rightCount(0) {}

  void begin()
  {
    motor.begin();
    melody.play("G4 100 20 C5 100 20 E5 100 20 G5 100 20 C6 100 20 D6 100 20 E6 200 200");
    roboEyes.begin(screenWidth, screenHeight, refreshDelay);
    setDefaultState();

    resetMovementHistory();
    lastActionTime = millis();
    lastMotorActionTime = millis();
    randomMotorInterval = random(1600, 10000);
  }

  void start()
  {
    if (isRunning)
      return;

    isRunning = true;
    lastActionTime = millis();
    lastMotorActionTime = millis();
    randomMotorInterval = random(1600, 10000);

    Serial.println("RobotPet: Started");
  }

  void stop()
  {
    if (!isRunning)
      return;

    isRunning = false;
    motor.stop();

    Serial.println("RobotPet: Stopped");
  }

  bool getRunningState()
  {
    return isRunning;
  }

  void update()
  {
    if (!isRunning)
    {
      return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - lastActionTime;
    unsigned long motorElapsed = now - lastMotorActionTime;

    switch (currentEyeState)
    {
    case Default:
      if (elapsed >= IDLE_DELAY)
      {
        unsigned int randomChoice = random(1, 11);
        if (randomChoice > 8)
          enterSleepyState();
        else if (randomChoice > 5)
          enterHappyState();
        else
          enterCuriosityState();

        lastActionTime = now;
      }
      if (motorElapsed >= randomMotorInterval)
      {
        randomMotorMovement();
        randomMotorInterval = random(1600, 10000);
        lastMotorActionTime = now;
      }
      break;

    case Angry:
      if (elapsed >= ANGRY_DURATION)
      {
        enterDefaultState();
        lastActionTime = now;
      }
      break;

    case Scare:
      if (elapsed >= SCARE_DURATION)
      {
        enterDefaultState();
        lastActionTime = now;
      }
      break;

    case Scared:
      if (elapsed >= SCARED_DURATION)
      {
        enterScareState();
        lastActionTime = now;
      }
      break;

    case Happy:
      if (elapsed >= HAPPY_DURATION)
      {
        enterDefaultState();
        lastActionTime = now;
      }
      break;

    case Curiosity:
      if (elapsed >= CURIOSITY_DURATION)
      {
        unsigned int randomChoice = random(1, 11);
        if (randomChoice > 8)
          enterSleepyState();
        else if (randomChoice > 5)
          enterHappyState();
        else
          enterCuriosityState();

        lastActionTime = now;
      }
      if (motorElapsed >= randomMotorInterval)
      {
        randomMotorMovement();
        randomMotorInterval = random(3200, 10000);
        lastMotorActionTime = now;
      }
      break;

    case Sleepy:
      if (elapsed >= DEEP_SLEEP_DELAY)
      {
        enterAsleepState();
        lastActionTime = now;
      }
      break;

    case Asleep:
      if (elapsed >= DEEP_SLEEP_DELAY)
      {
        unsigned int randomChoice = random(1, 11);
        if (randomChoice > 8)
          enterSleepyState();
        else
          enterDefaultState();

        lastActionTime = now;
        lastMotorActionTime = now;
      }
      break;
    }

    roboEyes.update();
  }

  void shortClick(int clickCount)
  {
    if (!isRunning)
      return;

    Serial.print("Click: ");
    Serial.println(clickCount);

    if ((currentEyeState == Asleep || currentEyeState == Sleepy))
    {
      enterAngryState();
      motor.backward();
      delay(100);
      motor.stop();
      lastActionTime = millis();
    }

    if (clickCount == 1)
    {
      if ((currentEyeState == Default || currentEyeState == Curiosity))
      {
        unsigned int randomChoice = random(1, 11);
        if (randomChoice > 7)
        {
          enterAngryState();
          motor.backward();
          delay(100);
          motor.stop();
        }
        else
        {
          enterHappyState();
        }
        lastActionTime = millis();
      }
    }
  }

  void longClick()
  {
    if (!isRunning)
      return;

    Serial.println("LONG PRESS!");
    if (currentEyeState == Default || currentEyeState == Curiosity)
    {
      enterLongHappyState();
      lastActionTime = millis();
    }
  }

  void longClickRelease()
  {
    if (!isRunning)
      return;

    Serial.println("LONG PRESS RELEASE!");
    currentEyeState = Happy;
  }
};

#endif