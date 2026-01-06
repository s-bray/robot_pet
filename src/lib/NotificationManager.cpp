#include "NotificationManager.h"

NotificationManager::NotificationManager(Adafruit_SSD1306 &disp, unsigned long duration)
    : display(disp),
      lineCount(0),
      notificationStartTime(0),
      displayDuration(duration),
      isActive(false),
      hasExpired(false),
      lastScrollTime(0)
{
  for (int i = 0; i < 5; i++)
  {
    scrollPositions[i] = 0;
  }
}

void NotificationManager::begin()
{
  appName = "";
  timestamp = "";
  lineCount = 0;
  isActive = false;
  hasExpired = false;
  resetScrollPositions();
}

void NotificationManager::resetScrollPositions()
{
  for (int i = 0; i < 5; i++)
  {
    scrollPositions[i] = 0;
  }
}

int NotificationManager::getTextWidth(String text)
{
  return text.length() * 6;
}

void NotificationManager::show(JsonDocument &doc)
{
  const char *app = doc["app"] | "Unknown";
  const char *time = doc["time"] | "";

  if (doc["texts"].is<JsonArray>())
  {
    show(app, time, doc["texts"].as<JsonArray>());
  }
}

void NotificationManager::show(const char *app, const char *time, JsonArray texts)
{
  appName = String(app);
  timestamp = String(time);
  lineCount = 0;

  for (JsonVariant text : texts)
  {
    if (lineCount >= 5)
      break;

    const char *textStr = text.as<const char *>();
    if (textStr)
    {
      textLines[lineCount] = String(textStr);
      lineCount++;
    }
  }

  isActive = true;
  hasExpired = false;
  notificationStartTime = millis();
  resetScrollPositions();

  Serial.println("=== NotificationManager ===");
  Serial.println("App: " + appName);
  Serial.println("Time: " + timestamp);
  Serial.println("Lines: " + String(lineCount));
  Serial.println("Duration: " + String(displayDuration) + "ms");
  Serial.println("====================");

  update();
}

void NotificationManager::drawAppIcon(const char *app)
{
  String appLower = String(app);
  appLower.toLowerCase();

  if (appLower.indexOf("whatsapp") >= 0)
  {
    drawWhatsAppIcon();
  }
  else if (appLower.indexOf("telegram") >= 0)
  {
    drawTelegramIcon();
  }
  else if (appLower.indexOf("gmail") >= 0 || appLower.indexOf("email") >= 0)
  {
    drawGmailIcon();
  }
  else
  {
    drawDefaultIcon();
  }
}

void NotificationManager::drawWhatsAppIcon()
{

  display.fillRoundRect(2, 2, 12, 10, 2, SSD1306_WHITE);
  display.fillTriangle(12, 10, 14, 12, 12, 12, SSD1306_WHITE);

  display.drawFastHLine(4, 5, 6, SSD1306_BLACK);
  display.drawFastHLine(4, 8, 6, SSD1306_BLACK);
}

void NotificationManager::drawTelegramIcon()
{

  display.fillTriangle(2, 8, 14, 2, 14, 14, SSD1306_WHITE);
  display.fillTriangle(8, 8, 14, 8, 11, 11, SSD1306_BLACK);
}

void NotificationManager::drawGmailIcon()
{

  display.drawRect(2, 4, 12, 8, SSD1306_WHITE);
  display.drawLine(2, 4, 8, 8, SSD1306_WHITE);
  display.drawLine(14, 4, 8, 8, SSD1306_WHITE);
}

void NotificationManager::drawDefaultIcon()
{

  display.fillRoundRect(5, 3, 6, 7, 2, SSD1306_WHITE);
  display.fillRect(7, 2, 2, 2, SSD1306_WHITE);
  display.fillRect(4, 10, 8, 2, SSD1306_WHITE);
  display.fillRect(6, 6, 4, 3, SSD1306_BLACK);
}

void NotificationManager::updateScrolling()
{
  unsigned long now = millis();
  if (now - lastScrollTime < SCROLL_DELAY)
    return;

  for (int i = 0; i < lineCount && i < 5; i++)
  {
    int textWidth = getTextWidth(textLines[i]);
    int availableWidth = SCREEN_WIDTH - 18;

    if (textWidth > availableWidth)
    {
      scrollPositions[i] += 2;

      if (scrollPositions[i] > textWidth + 12)
      {
        scrollPositions[i] = 0;
      }
    }
  }

  lastScrollTime = now;
}

void NotificationManager::update()
{
  if (!isActive)
    return;

  unsigned long now = millis();
  unsigned long elapsed = now - notificationStartTime;

  if (elapsed >= displayDuration)
  {
    hasExpired = true;
    Serial.println("[NotificationManager] Expired after " + String(elapsed) + "ms");
    return;
  }

  display.clearDisplay();

  drawAppIcon(appName.c_str());

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  int appNameWidth = getTextWidth(appName);
  if (appNameWidth > 96)
  {

    int xPos = 18 - (scrollPositions[0] % (appNameWidth + 12));
    display.setCursor(xPos, 0);
    display.print(appName + "   ");
    display.setCursor(xPos + appNameWidth + 12, 0);
    display.print(appName + "   ");
  }
  else
  {
    display.setCursor(18, 0);
    display.print(appName);
  }

  if (timestamp.length() > 0)
  {
    display.setTextSize(1);
    int timeWidth = timestamp.length() * 6;
    int timeX = SCREEN_WIDTH - timeWidth - 2;
    display.setCursor(timeX, 12);
    display.print(timestamp.substring(11));
  }

  display.drawFastHLine(0, 22, SCREEN_WIDTH, SSD1306_WHITE);

  int yPos = 26;
  for (int i = 0; i < lineCount && i < 3; i++)
  {
    if (yPos >= SCREEN_HEIGHT - 8)
      break;

    int textWidth = getTextWidth(textLines[i]);
    int availableWidth = SCREEN_WIDTH - 4;

    if (textWidth > availableWidth)
    {

      int loopWidth = textWidth + 18;
      int xPos = 2 - (scrollPositions[i + 1] % loopWidth);

      display.setCursor(xPos, yPos);
      display.print(textLines[i] + "   ");

      display.setCursor(xPos + loopWidth, yPos);
      display.print(textLines[i] + "   ");
    }
    else
    {

      display.setCursor(2, yPos);
      display.print(textLines[i]);
    }

    yPos += LINE_HEIGHT;
  }

  int progressWidth = map(elapsed, 0, displayDuration, 0, SCREEN_WIDTH);
  display.drawFastHLine(0, SCREEN_HEIGHT - 2, progressWidth, SSD1306_WHITE);

  updateScrolling();

  display.setTextWrap(true);
  display.display();
}

void NotificationManager::dismiss()
{
  isActive = false;
  hasExpired = true;
  display.clearDisplay();
  display.display();
  Serial.println("[NotificationManager] Dismissed manually");
}

bool NotificationManager::isExpired()
{
  return hasExpired;
}

bool NotificationManager::isShowing()
{
  return isActive && !hasExpired;
}

unsigned long NotificationManager::getRemainingTime()
{
  if (!isActive || hasExpired)
    return 0;

  unsigned long elapsed = millis() - notificationStartTime;
  if (elapsed >= displayDuration)
    return 0;

  return displayDuration - elapsed;
}