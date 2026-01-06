#pragma once
#include <Arduino.h>

class SoundPlayer
{
public:
  SoundPlayer(uint8_t buzzerPin) : pin(buzzerPin)
  {
    pinMode(pin, OUTPUT);
  }

  void play(const char *sequence)
  {
    Serial.println("Melody Play");
    const char *p = sequence;

    while (*p)
    {
      char noteChar = *p++;
      if (noteChar < 'A' || noteChar > 'G')
        continue;

      int octave = (*p >= '0' && *p <= '9') ? (*p++ - '0') : 5;

      int freq = noteToFreq(noteChar, octave);

      int duration = parseNumber(p);
      int pause = parseNumber(p);

      tone(pin, freq, duration);
      delay(duration + pause);
    }
  }

private:
  uint8_t pin;

  int parseNumber(const char *&p)
  {
    while (*p == ' ')
      p++;
    int value = 0;
    while (*p >= '0' && *p <= '9')
    {
      value = value * 10 + (*p++ - '0');
    }
    return value;
  }

  int noteToFreq(char note, int octave)
  {
    static const int baseFreq[] = {
        0,
        262,
        294,
        330,
        349,
        392,
        440,
        494};

    int index = (note - 'A' + 1);
    if (index < 1 || index > 7)
      return 0;

    float freq = baseFreq[index];

    int diff = octave - 4;
    while (diff > 0)
    {
      freq *= 2;
      diff--;
    }
    while (diff < 0)
    {
      freq /= 2;
      diff++;
    }

    return (int)freq;
  }
};
