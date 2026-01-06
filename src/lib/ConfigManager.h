#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

struct WiFiConfig
{
  String ssid;
  String password;
  bool isValid;
};

struct SettingConfig
{
  bool bluetooth;
  bool wifi;
};

class ConfigManager
{
private:
  Preferences preferences;
  static const char *WIFI_NAMESPACE;
  static const char *SETTINGS_NAMESPACE;
  static const int MAX_WIFI_NETWORKS = 10;

public:
  ConfigManager();
  ~ConfigManager();

  bool addWiFiConfig(const String &ssid, const String &password);
  std::vector<WiFiConfig> loadAllWiFiConfigs();
  bool removeWiFiConfig(const String &ssid);
  void clearAllWiFiConfigs();
  int getWiFiCount();

  void saveWiFiConfig(const String &ssid, const String &password);
  WiFiConfig loadWiFiConfig();
  void clearWiFiConfig();

  String getDeviceID();

  void saveSettingsConfig(const String &key, const bool &value);
  SettingConfig loadSettingsConfig();
};

#endif