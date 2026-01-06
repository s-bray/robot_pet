#include "MediaVisualizer.h"

MediaVisualizer::MediaVisualizer(Adafruit_SSD1306 &disp, FrameRate frameRate)
    : display(disp),
      currentAmplitude(0),
      peakValue(0),
      rmsValue(0),
      isPlaying(false),
      hasValidMetadata(false),
      titleScrollPos(0),
      artistScrollPos(0),
      lastScrollTime(0),
      lastUpdateTime(0),
      lastAmplitudeReceived(0),
      isActive(false),
      targetFrameRate(frameRate)
{

  updateInterval = 1000 / targetFrameRate;

  for (int i = 0; i < NUM_BARS; i++)
  {
    barHeights[i] = 0;
    barTargets[i] = 0;
    barVelocities[i] = 0;
    peakPositions[i] = 0;
    peakTimers[i] = 0;
  }
}

void MediaVisualizer::begin()
{
  mediaTitle = "";
  mediaArtist = "";
  mediaStatus = "";
  isPlaying = false;
  hasValidMetadata = false;
  isActive = false;
}

bool MediaVisualizer::checkValidMetadata()
{
  bool hasTitle = (mediaTitle.length() > 0 && mediaTitle != "Unknown");
  bool hasArtist = (mediaArtist.length() > 0 && mediaArtist != "Unknown");
  return hasTitle || hasArtist;
}

int MediaVisualizer::getVisualizerHeight()
{
  return hasValidMetadata ? VISUALIZER_HEIGHT_WITH_METADATA : VISUALIZER_HEIGHT_FULLSCREEN;
}

int MediaVisualizer::getVisualizerYStart()
{
  return hasValidMetadata ? VISUALIZER_Y_START_WITH_METADATA : VISUALIZER_Y_START_FULLSCREEN;
}

void MediaVisualizer::drawMetadata()
{
  if (!hasValidMetadata)
    return;

  if (isPlaying)
  {
    display.fillTriangle(2, 2, 2, 8, 7, 5, SSD1306_WHITE);
  }
  else
  {
    display.fillRect(2, 2, 2, 6, SSD1306_WHITE);
    display.fillRect(6, 2, 2, 6, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  int availableWidth = SCREEN_WIDTH - TEXT_MARGIN_LEFT - 2;

  if (mediaTitle.length() > 0 && mediaTitle != "Unknown")
  {
    String displayTitle = mediaTitle;
    int titleWidth = displayTitle.length() * 6;

    if (titleWidth > availableWidth)
    {
      int loopWidth = titleWidth + 18;
      int xPos = TEXT_MARGIN_LEFT - (titleScrollPos % loopWidth);

      display.setCursor(xPos, 0);
      display.print(displayTitle + "   ");

      display.setCursor(xPos + loopWidth, 0);
      display.print(displayTitle + "   ");

      display.fillRect(0, 0, TEXT_MARGIN_LEFT - 1, 9, SSD1306_BLACK);

      if (isPlaying)
      {
        display.fillTriangle(2, 2, 2, 8, 7, 5, SSD1306_WHITE);
      }
      else
      {
        display.fillRect(2, 2, 2, 6, SSD1306_WHITE);
        display.fillRect(6, 2, 2, 6, SSD1306_WHITE);
      }
    }
    else
    {
      int centerPos = TEXT_MARGIN_LEFT + (availableWidth - titleWidth) / 2;
      display.setCursor(centerPos, 0);
      display.print(displayTitle);
    }
  }

  if (mediaArtist.length() > 0 && mediaArtist != "Unknown")
  {
    int artistWidth = mediaArtist.length() * 6;
    if (artistWidth > availableWidth)
    {
      int loopWidth = artistWidth + 18;
      int xPos = TEXT_MARGIN_LEFT - (artistScrollPos % loopWidth);

      display.setCursor(xPos, 10);
      display.print(mediaArtist + "   ");

      display.setCursor(xPos + loopWidth, 10);
      display.print(mediaArtist + "   ");

      display.fillRect(0, 10, TEXT_MARGIN_LEFT - 1, 9, SSD1306_BLACK);
    }
    else
    {
      int centerPos = TEXT_MARGIN_LEFT + (availableWidth - artistWidth) / 2;
      display.setCursor(centerPos, 10);
      display.print(mediaArtist);
    }
  }

  display.setTextWrap(true);
}

void MediaVisualizer::drawVisualizer()
{
  int visualizerHeight = getVisualizerHeight();
  int visualizerYStart = getVisualizerYStart();
  int barWidth = SCREEN_WIDTH / NUM_BARS;
  int spacing = 1;
  int actualBarWidth = barWidth - spacing;

  for (int i = 0; i < NUM_BARS; i++)
  {
    int x = i * barWidth;

    float diff = barTargets[i] - barHeights[i];
    barVelocities[i] = barVelocities[i] * 0.7 + diff * 0.3;
    barHeights[i] += barVelocities[i];

    if (barHeights[i] < 0)
      barHeights[i] = 0;
    if (barHeights[i] > visualizerHeight)
      barHeights[i] = visualizerHeight;

    int barHeight = (int)barHeights[i];
    int y = visualizerYStart + (visualizerHeight - barHeight);

    if (barHeight > 0)
    {
      display.fillRect(x, y, actualBarWidth, barHeight, SSD1306_WHITE);
    }

    if (peakPositions[i] > 0)
    {
      int peakY = visualizerYStart + (visualizerHeight - peakPositions[i]);
      display.drawFastHLine(x, peakY, actualBarWidth, SSD1306_WHITE);

      if (millis() - peakTimers[i] > 50)
      {
        peakPositions[i]--;
        peakTimers[i] = millis();
      }
    }

    if (barHeight > peakPositions[i])
    {
      peakPositions[i] = barHeight;
      peakTimers[i] = millis();
    }
  }
}

void MediaVisualizer::updateScrolling()
{
  if (!hasValidMetadata)
    return;

  unsigned long now = millis();
  if (now - lastScrollTime >= SCROLL_DELAY)
  {
    int availableWidth = SCREEN_WIDTH - TEXT_MARGIN_LEFT - 2;

    int titleWidth = mediaTitle.length() * 6;
    if (titleWidth > availableWidth)
    {
      titleScrollPos += 2;
    }

    int artistWidth = mediaArtist.length() * 6;
    if (artistWidth > availableWidth)
    {
      artistScrollPos += 2;
    }

    lastScrollTime = now;
  }
}

void MediaVisualizer::generateBarTargets()
{
  int visualizerHeight = getVisualizerHeight();
  float baseHeight = currentAmplitude * visualizerHeight;

  for (int i = 0; i < NUM_BARS; i++)
  {
    float phase = (float)i / NUM_BARS * 3.14159 * 2;
    float wave = sin(phase + millis() * 0.002) * 0.3 + 1.0;
    float randomFactor = (random(80, 120) / 100.0);

    float target = baseHeight * wave * randomFactor;

    if (peakValue > currentAmplitude)
    {
      target += (peakValue - currentAmplitude) * visualizerHeight * 0.5;
    }

    barTargets[i] = constrain(target, 0, visualizerHeight);
  }
}

void MediaVisualizer::setFrameRate(FrameRate frameRate)
{
  targetFrameRate = frameRate;
  updateInterval = 1000 / targetFrameRate;
  Serial.print("Frame rate set to: ");
  Serial.print(targetFrameRate);
  Serial.println(" fps");
}

FrameRate MediaVisualizer::getFrameRate()
{
  return targetFrameRate;
}

void MediaVisualizer::handleMediaData(JsonDocument &doc)
{
  const char *type = doc["type"];

  if (strcmp(type, "media") == 0)
  {
    String newTitle = doc["title"] | "";
    String newArtist = doc["artist"] | "";

    bool titleChanged = (newTitle != mediaTitle);
    bool artistChanged = (newArtist != mediaArtist);

    mediaTitle = newTitle;
    mediaArtist = newArtist;
    mediaStatus = doc["status"] | "";
    isPlaying = doc["is_playing"] | false;

    hasValidMetadata = checkValidMetadata();

    if (titleChanged)
    {
      titleScrollPos = 0;
    }

    if (artistChanged)
    {
      artistScrollPos = 0;
    }

    bool hasAmplitude = false;
    if (doc["audio_amplitude"].is<JsonObject>())
    {
      JsonObject audioAmp = doc["audio_amplitude"];
      currentAmplitude = audioAmp["amplitude"] | 0.0f;
      peakValue = audioAmp["peak"] | 0.0f;
      rmsValue = audioAmp["rms"] | 0.0f;

      lastAmplitudeReceived = millis();
      hasAmplitude = (currentAmplitude > 0.0f || peakValue > 0.0f);
    }

    if (hasAmplitude || hasValidMetadata)
    {
      isActive = true;
    }

    if (titleChanged || artistChanged)
    {
      Serial.println("=== Media Updated ===");
      Serial.println("Title: " + (mediaTitle.length() > 0 ? mediaTitle : "(empty)"));
      Serial.println("Artist: " + (mediaArtist.length() > 0 ? mediaArtist : "(empty)"));
      Serial.println("Has Valid Metadata: " + String(hasValidMetadata ? "YES" : "NO"));
      Serial.println("Mode: " + String(hasValidMetadata ? "WITH METADATA" : "VISUALIZER ONLY"));
      Serial.println("====================");
    }
  }
}

void MediaVisualizer::update()
{
  if (!isActive)
    return;

  unsigned long now = millis();

  if (now - lastAmplitudeReceived > AMPLITUDE_TIMEOUT)
  {
    currentAmplitude = 0;
    peakValue = 0;
    rmsValue = 0;
  }

  if (now - lastUpdateTime >= updateInterval)
  {
    display.clearDisplay();

    if (hasValidMetadata)
    {
      drawMetadata();

      display.drawFastHLine(0, 19, SCREEN_WIDTH, SSD1306_WHITE);
    }

    if (currentAmplitude > 0.01 || peakValue > 0.01)
    {
      generateBarTargets();
    }
    else if (isPlaying)
    {
      generateBarTargets();
    }
    else
    {

      for (int i = 0; i < NUM_BARS; i++)
      {
        barTargets[i] *= 0.95;
      }
    }

    drawVisualizer();
    updateScrolling();

    display.display();
    lastUpdateTime = now;
  }
}

void MediaVisualizer::stop()
{
  isActive = false;
  display.clearDisplay();
  display.display();
}

bool MediaVisualizer::isVisualizerActive()
{
  return isActive;
}

void MediaVisualizer::setAmplitude(float amplitude)
{
  currentAmplitude = constrain(amplitude, 0.0f, 1.0f);
  lastAmplitudeReceived = millis();
}

void MediaVisualizer::setPeak(float peak)
{
  peakValue = constrain(peak, 0.0f, 1.0f);
}

void MediaVisualizer::activateVisualizerOnly()
{
  hasValidMetadata = false;
  isActive = true;
  Serial.println("Visualizer activated: FULLSCREEN MODE");
}