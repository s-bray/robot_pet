#ifndef MEDIA_VISUALIZER_H
#define MEDIA_VISUALIZER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

enum FrameRate
{
  FPS_30 = 30,
  FPS_60 = 60
};

class MediaVisualizer
{
private:
  Adafruit_SSD1306 &display;

  String mediaTitle;
  String mediaArtist;
  String mediaStatus;
  bool isPlaying;
  bool hasValidMetadata;

  float currentAmplitude;
  float peakValue;
  float rmsValue;

  static const int NUM_BARS = 16;
  float barHeights[NUM_BARS];
  float barTargets[NUM_BARS];
  float barVelocities[NUM_BARS];

  int peakPositions[NUM_BARS];
  unsigned long peakTimers[NUM_BARS];

  int titleScrollPos;
  int artistScrollPos;
  unsigned long lastScrollTime;
  static const unsigned long SCROLL_DELAY = 100;

  const int SCREEN_WIDTH = 128;
  const int SCREEN_HEIGHT = 64;
  const int METADATA_HEIGHT = 20;
  const int VISUALIZER_HEIGHT_WITH_METADATA = 44;
  const int VISUALIZER_HEIGHT_FULLSCREEN = 64;
  const int VISUALIZER_Y_START_WITH_METADATA = 20;
  const int VISUALIZER_Y_START_FULLSCREEN = 0;
  const int TEXT_MARGIN_LEFT = 12;

  FrameRate targetFrameRate;
  unsigned long updateInterval;
  unsigned long lastUpdateTime;

  unsigned long lastAmplitudeReceived;
  static const unsigned long AMPLITUDE_TIMEOUT = 500;

  bool isActive;

  bool checkValidMetadata();
  int getVisualizerHeight();
  int getVisualizerYStart();
  void drawMetadata();
  void drawVisualizer();
  void updateScrolling();
  void generateBarTargets();

public:
  MediaVisualizer(Adafruit_SSD1306 &disp, FrameRate frameRate = FPS_30);

  void begin();
  void setFrameRate(FrameRate frameRate);
  FrameRate getFrameRate();
  void handleMediaData(JsonDocument &doc);
  void update();
  void stop();
  bool isVisualizerActive();
  void setAmplitude(float amplitude);
  void setPeak(float peak);
  void activateVisualizerOnly();
};

#endif