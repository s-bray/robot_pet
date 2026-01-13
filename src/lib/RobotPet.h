#ifndef ROBOT_PET_H
#define ROBOT_PET_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <lib/FluxGarage_RoboEyes.h>
#include "IMUManager.h"
#include "ServoManager.h"

class RobotPet
{
private:
  Adafruit_SSD1306 &display;
  RoboEyes<Adafruit_SSD1306> roboEyes;
  IMUManager &imu;
  ServoManager &servo;

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
    Angry,
    Excited,
    Dizzy,
    Touched
  } currentEyeState;

  static const unsigned long IDLE_DELAY = 5000;
  static const unsigned long DEEP_SLEEP_DELAY = 10000;
  static const unsigned long ANGRY_DURATION = 5000;
  static const unsigned long HAPPY_DURATION = 500;
  static const unsigned long CURIOSITY_DURATION = 1000;
  static const unsigned long SCARE_DURATION = 4000;
  static const unsigned long SCARED_DURATION = 2000;
  static const unsigned long EXCITED_DURATION = 300;
  static const unsigned long DIZZY_DURATION = 4000;
  static const unsigned long TOUCHED_DURATION = 300;
  static const unsigned long PICKUP_COOLDOWN = 500;

  unsigned long lastActionTime;
  unsigned long lastPickupTime;

  void setDefaultState()
  {
    roboEyes.setMood(DEFAULT);
    roboEyes.setWidth(30, 30);
    roboEyes.setHeight(24, 24); // Resize for 32px height
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



  // Moved enterDefaultState to public section

  void enterTouchedState()
  {
    currentEyeState = Touched;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(36, 36);
    roboEyes.setHeight(20, 20); // Squinty happy
    roboEyes.setBorderradius(10, 10);
    roboEyes.anim_laugh();
    roboEyes.setIdleMode(OFF);
    roboEyes.setAutoblinker(OFF);
    
    // User requested NO movement for other states including Touched?
    // "only want the servomovements in happy and excited, and very very slow in curiosity"
    // So I will STOP servos here.
    servo.stop(); 
    
    Serial.println("CurrentState: Touched");
  }

  void enterHappyState()
  {
    currentEyeState = Happy;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36); // Restored original size
    roboEyes.setBorderradius(8, 8);
    roboEyes.anim_laugh();
    roboEyes.setIdleMode(OFF);
    roboEyes.setAutoblinker(OFF);
    
    // Normal wiggle for general happiness (not direct touch)
    servo.setWiggleSpeed(200);
    servo.startWiggle();
    
    Serial.println("CurrentState: Happy");
  }

  void enterLongHappyState()
  {
    currentEyeState = LongHappy;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36); // Restored original size
    roboEyes.setBorderradius(8, 8);
    roboEyes.setVFlicker(ON, 5);
    roboEyes.setIdleMode(OFF);
    roboEyes.setAutoblinker(OFF);
    
    // Long press = FAST wiggle too
    servo.setWiggleSpeed(100);
    servo.startWiggle();
    
    Serial.println("CurrentState: LongHappy");
  }

  void enterScaredState()
  {
    currentEyeState = Scared;
    roboEyes.setMood(TIRED);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(24, 24);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setSweat(ON);
    roboEyes.setVFlicker(ON, 3);
    roboEyes.setHFlicker(ON, 3);
    roboEyes.setAutoblinker(OFF);
    servo.stop(); // No movement
    Serial.println("CurrentState: Scared");
  }

  void enterScareState()
  {
    currentEyeState = Scare;
    roboEyes.setMood(TIRED);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(24, 24);
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setSweat(OFF);
    roboEyes.setVFlicker(ON, 3);
    roboEyes.setHFlicker(ON, 3);
    roboEyes.setAutoblinker(OFF);
    servo.stop(); // No movement
    Serial.println("CurrentState: Scare");
  }

  void enterCuriosityState()
  {
    currentEyeState = Curiosity;
    roboEyes.setMood(DEFAULT);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36); // Restored original size
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setAutoblinker(ON, 2, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    roboEyes.setHFlicker(OFF);
    roboEyes.setSweat(OFF);
    roboEyes.setCuriosity(ON);
    
    // VERY slow wiggle for curiosity (User requested "very very slow")
    servo.setWiggleSpeed(600); 
    servo.startWiggle();
    
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
    servo.stop(); // No movement
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
    servo.stop(); // No movement
    Serial.println("CurrentState: Asleep");
  }

  void enterAngryState()
  {
    currentEyeState = Angry;
    roboEyes.setMood(ANGRY);
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(36, 36); // Restored original size
    roboEyes.setBorderradius(8, 8);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setHFlicker(ON, 2);
    servo.stop(); // No movement
    Serial.println("CurrentState: Angry");
  }
  
  void enterExcitedState()
  {
    currentEyeState = Excited;
    roboEyes.setMood(HAPPY);
    roboEyes.setWidth(36, 36);
    roboEyes.setHeight(28, 28);
    roboEyes.setBorderradius(12, 12);
    roboEyes.setAutoblinker(ON, 1, 1); // Fast blinking
    roboEyes.setHFlicker(ON, 1); // Slight jitter
    
    servo.setWiggleSpeed(150); // Fast wiggle for pickup
    servo.startWiggle();
    
    Serial.println("CurrentState: Excited");
  }
  
  void enterDizzyState()
  {
    currentEyeState = Dizzy;
    roboEyes.setMood(TIRED); // Droopy eyes
    roboEyes.setWidth(32, 32);
    roboEyes.setHeight(24, 24);
    roboEyes.setSpacebetween(10); // Eyes closer together
    roboEyes.setBorderradius(12, 12); // Round
    // Simulation of rolling eyes managed in update() or just use flicker for now
    roboEyes.setVFlicker(ON, 8);
    roboEyes.setHFlicker(ON, 8);
    servo.stop(); // No movement
    Serial.println("CurrentState: Dizzy");
    
    servo.setWiggleSpeed(200);
    servo.startWiggle(); 
  }

public:
  RobotPet(Adafruit_SSD1306 &disp, IMUManager &imuMgr, ServoManager &servoMgr, int width, int heigh, int delay)
      : display(disp), roboEyes(disp), imu(imuMgr), servo(servoMgr),
        screenWidth(width), screenHeight(heigh), refreshDelay(delay),
        currentEyeState(Default), lastActionTime(0), lastPickupTime(0), isRunning(false) {}

  void begin()
  {
    imu.begin();
    servo.begin();
    roboEyes.begin(screenWidth, screenHeight, refreshDelay);
    setDefaultState();

    lastActionTime = millis();
  }

  // The "Dictionary of Emotions" for the External Brain
  void setMood(String emotion)
  {
    emotion.toUpperCase();
    Serial.print("Cmd: "); Serial.println(emotion);

    if (emotion == "HAPPY") enterHappyState();
    else if (emotion == "ANGRY") enterAngryState();
    else if (emotion == "SCARED") enterScaredState();
    else if (emotion == "SCARE") enterScareState(); // Alternate naming
    else if (emotion == "EXCITED") enterExcitedState();
    else if (emotion == "CURIOUS" || emotion == "CURIOSITY") enterCuriosityState();
    else if (emotion == "SLEEPY") enterSleepyState();
    else if (emotion == "ASLEEP" || emotion == "SLEEP") enterAsleepState();
    else if (emotion == "DIZZY") enterDizzyState();
    else if (emotion == "DEFAULT" || emotion == "IDLE") enterDefaultState();
    else Serial.println("Unknown Emotion");
  }

  void enterDefaultState()
  {
    servo.stop();
    currentEyeState = Default;
    setDefaultState();
    Serial.println("CurrentState: Default");
  }

  void start()
  {
    if (isRunning)
      return;

    isRunning = true;
    lastActionTime = millis();

    Serial.println("RobotPet: Started");
    enterDefaultState(); // Ensure state is logged & reset
  }

  void stop()
  {
    if (!isRunning)
      return;

    isRunning = false;
    servo.stop();
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
    
    imu.update();
    servo.update();

    // Check for high priority physical interactions/interrupts
    // Only check IMU sensors when in non-animated states
    if (currentEyeState == Default || currentEyeState == Scared || currentEyeState == Scare || currentEyeState == Angry) {
       if (imu.isShaken()) {
         Serial.println(">>> SHAKEN DETECTED <<<");
          enterDizzyState();
          lastActionTime = millis();
         return; // Skip rest of logic
       }
       // Pickup detection with cooldown
       if (imu.isPickedUp() && (millis() - lastPickupTime >= PICKUP_COOLDOWN)) {
          Serial.println(">>> PICKUP DETECTED <<<");
           enterExcitedState();
           lastActionTime = millis();
           lastPickupTime = millis(); // Start cooldown
          return; // Skip rest of logic
       }
    }

    unsigned long now = millis();
    unsigned long elapsed = now - lastActionTime;

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
      
    case Touched:
      if (elapsed >= TOUCHED_DURATION)
      {
          enterDefaultState();
          lastActionTime = now;
      }
      break;
      
    case LongHappy:
       // Logic to stay here? Or timeout?
       // Usually controlled by LongPressRelease, but safe to timeout
       if (elapsed >= HAPPY_DURATION * 4) {
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
      }
      break;
      
   case Excited:
      if (elapsed >= EXCITED_DURATION)
      {
          enterDefaultState();
          lastActionTime = now;
      }
      break;
      
    case Dizzy:
      if (elapsed >= DIZZY_DURATION)
      {
          enterDefaultState();
          lastActionTime = now;
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

    // Touch ALWAYS overrides current state
    enterTouchedState();
    lastActionTime = millis();
  }

  void longClick()
  {
    if (!isRunning)
      return;

    Serial.println("LONG PRESS!");
    // Long press ALWAYS overrides
    enterLongHappyState();
    lastActionTime = millis();
  }

  void longClickRelease()
  {
    if (!isRunning)
      return;

    Serial.println("LONG PRESS RELEASE!");
    enterTouchedState(); // Transition to Touched for a bit after release?
  }
};

#endif