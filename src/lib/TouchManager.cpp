#include "TouchManager.h"

TouchManager::TouchManager(int pin)
    : pin(pin),
      lastState(LOW),
      lastChangeTime(0),
      touchDownTime(0),
      clickCount(0),
      longPressFired(false),
      longPressActive(false)
{
}

void TouchManager::begin()
{
  pinMode(pin, INPUT);
}

void TouchManager::addClickCallback(std::function<void(int)> cb)
{
  clickCallbacks.push_back(cb);
}

void TouchManager::addLongPressCallback(std::function<void()> cb)
{
  longPressCallbacks.push_back(cb);
}

void TouchManager::addLongPressReleaseCallback(std::function<void()> cb)
{
  longPressReleaseCallbacks.push_back(cb);
}

void TouchManager::update()
{
  bool reading = digitalRead(pin);
  unsigned long now = millis();

  if (reading != lastState)
  {
    lastChangeTime = now;
    lastState = reading;

    if (reading == HIGH)
    {
      touchDownTime = now;
      longPressFired = false;
      longPressActive = false;
    }
  }

  if (reading == LOW && (now - lastChangeTime) > debounceDelay)
  {
    if (longPressActive)
    {
      onLongPressRelease();
      longPressActive = false;
      touchDownTime = 0;
      return;
    }

    if (touchDownTime > 0 && !longPressFired)
      clickCount++;

    touchDownTime = 0;
  }

  if (reading == HIGH &&
      touchDownTime > 0 &&
      !longPressFired &&
      (now - touchDownTime >= longPressTime))
  {
    longPressFired = true;
    longPressActive = true;
    clickCount = 0;
    onLongPress();
  }

  if (clickCount > 0 &&
      (now - lastChangeTime) > multiClickDelay)
  {
    onClick(clickCount);
    clickCount = 0;
  }
}

void TouchManager::onClick(int count)
{
  for (auto &cb : clickCallbacks)
    if (cb)
      cb(count);
}

void TouchManager::onLongPress()
{
  for (auto &cb : longPressCallbacks)
    if (cb)
      cb();
}

void TouchManager::onLongPressRelease()
{
  for (auto &cb : longPressReleaseCallbacks)
    if (cb)
      cb();
}
