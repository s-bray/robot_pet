#include "ButtonManager.h"

ButtonManager::ButtonManager(int pin)
    : pin(pin),
      lastState(LOW),
      lastChangeTime(0),
      buttonDownTime(0),
      clickCount(0),
      longPressFired(false),
      longPressActive(false)
{
}

void ButtonManager::begin()
{
  pinMode(pin, INPUT_PULLUP);
}

void ButtonManager::addClickCallback(std::function<void(int)> cb)
{
  clickCallbacks.push_back(cb);
}

void ButtonManager::addLongPressCallback(std::function<void()> cb)
{
  longPressCallbacks.push_back(cb);
}

void ButtonManager::addLongPressReleaseCallback(std::function<void()> cb)
{
  longPressReleaseCallbacks.push_back(cb);
}

void ButtonManager::update()
{
  bool reading = digitalRead(pin);
  unsigned long now = millis();

  if (reading != lastState)
  {
    lastChangeTime = now;
    lastState = reading;

    if (reading == HIGH)
    {
      buttonDownTime = now;
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
      buttonDownTime = 0;
      return;
    }

    if (buttonDownTime > 0 && !longPressFired)
      clickCount++;

    buttonDownTime = 0;
  }

  if (reading == HIGH &&
      buttonDownTime > 0 &&
      !longPressFired &&
      (now - buttonDownTime >= longPressTime))
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

void ButtonManager::onClick(int count)
{
  for (auto &cb : clickCallbacks)
    if (cb)
      cb(count);
}

void ButtonManager::onLongPress()
{
  for (auto &cb : longPressCallbacks)
    if (cb)
      cb();
}

void ButtonManager::onLongPressRelease()
{
  for (auto &cb : longPressReleaseCallbacks)
    if (cb)
      cb();
}
