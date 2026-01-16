#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <functional>
#include <vector>

class TouchManager
{
private:
  int pin;
  bool lastState;
  unsigned long lastChangeTime;
  unsigned long touchDownTime;
  int clickCount;
  bool longPressFired;
  bool longPressActive;

  const unsigned long debounceDelay = 10;
  const unsigned long multiClickDelay = 300;
  const unsigned long longPressTime = 800;

  std::vector<std::function<void(int)>> clickCallbacks;
  std::vector<std::function<void()>> longPressCallbacks;
  std::vector<std::function<void()>> longPressReleaseCallbacks;
  std::vector<std::function<void()>> touchDownCallbacks;

public:
  TouchManager(int pin);
  void begin();
  void update();

  void addClickCallback(std::function<void(int)> cb);
  void addLongPressCallback(std::function<void()> cb);
  void addLongPressReleaseCallback(std::function<void()> cb);
  void addTouchDownCallback(std::function<void()> cb);

private:
  void onClick(int count);
  void onLongPress();
  void onLongPressRelease();
  void onTouchDown();
};

#endif
