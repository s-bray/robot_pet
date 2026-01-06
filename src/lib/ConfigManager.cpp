#include "ConfigManager.h"

const char *ConfigManager::WIFI_NAMESPACE = "wifi";
const char *ConfigManager::SETTINGS_NAMESPACE = "settings";

ConfigManager::ConfigManager() {}

ConfigManager::~ConfigManager()
{
  preferences.end();
}

bool ConfigManager::addWiFiConfig(const String &ssid, const String &password)
{
  preferences.begin(WIFI_NAMESPACE, false);

  int count = preferences.getInt("count", 0);

  for (int i = 0; i < count; i++)
  {
    String key = "ssid_" + String(i);
    String existingSSID = preferences.getString(key.c_str(), "");

    if (existingSSID == ssid)
    {
      String passKey = "pass_" + String(i);
      preferences.putString(passKey.c_str(), password);
      preferences.end();

      Serial.printf("✅ WiFi '%s' updated\n", ssid.c_str());
      return true;
    }
  }

  if (count >= MAX_WIFI_NETWORKS)
  {
    preferences.end();
    Serial.println("❌ Sudah mencapai maksimal WiFi networks!");
    return false;
  }

  String ssidKey = "ssid_" + String(count);
  String passKey = "pass_" + String(count);

  preferences.putString(ssidKey.c_str(), ssid);
  preferences.putString(passKey.c_str(), password);
  preferences.putInt("count", count + 1);
  preferences.end();

  Serial.printf("✅ WiFi #%d saved: %s\n", count + 1, ssid.c_str());
  return true;
}

std::vector<WiFiConfig> ConfigManager::loadAllWiFiConfigs()
{
  std::vector<WiFiConfig> configs;

  preferences.begin(WIFI_NAMESPACE, true);
  int count = preferences.getInt("count", 0);

  for (int i = 0; i < count; i++)
  {
    WiFiConfig config;

    String ssidKey = "ssid_" + String(i);
    String passKey = "pass_" + String(i);

    config.ssid = preferences.getString(ssidKey.c_str(), "");
    config.password = preferences.getString(passKey.c_str(), "");
    config.isValid = !config.ssid.isEmpty();

    if (config.isValid)
    {
      configs.push_back(config);
    }
  }

  preferences.end();
  return configs;
}

bool ConfigManager::removeWiFiConfig(const String &ssid)
{
  preferences.begin(WIFI_NAMESPACE, false);
  int count = preferences.getInt("count", 0);

  for (int i = 0; i < count; i++)
  {
    String ssidKey = "ssid_" + String(i);
    String existingSSID = preferences.getString(ssidKey.c_str(), "");

    if (existingSSID == ssid)
    {
      for (int j = i; j < count - 1; j++)
      {
        String srcSSID = "ssid_" + String(j + 1);
        String srcPass = "pass_" + String(j + 1);
        String dstSSID = "ssid_" + String(j);
        String dstPass = "pass_" + String(j);

        preferences.putString(dstSSID.c_str(), preferences.getString(srcSSID.c_str(), ""));
        preferences.putString(dstPass.c_str(), preferences.getString(srcPass.c_str(), ""));
      }

      String lastSSID = "ssid_" + String(count - 1);
      String lastPass = "pass_" + String(count - 1);
      preferences.remove(lastSSID.c_str());
      preferences.remove(lastPass.c_str());

      preferences.putInt("count", count - 1);
      preferences.end();

      Serial.printf("🗑️ WiFi '%s' dihapus\n", ssid.c_str());
      return true;
    }
  }

  preferences.end();
  Serial.printf("❌ WiFi '%s' tidak ditemukan\n", ssid.c_str());
  return false;
}

void ConfigManager::clearAllWiFiConfigs()
{
  preferences.begin(WIFI_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  Serial.println("🗑️ Semua WiFi configs dihapus");
}

int ConfigManager::getWiFiCount()
{
  preferences.begin(WIFI_NAMESPACE, true);
  int count = preferences.getInt("count", 0);
  preferences.end();
  return count;
}

void ConfigManager::saveWiFiConfig(const String &ssid, const String &password)
{
  addWiFiConfig(ssid, password);
}

WiFiConfig ConfigManager::loadWiFiConfig()
{
  WiFiConfig config;
  std::vector<WiFiConfig> configs = loadAllWiFiConfigs();

  if (!configs.empty())
  {
    config = configs[0];
  }
  else
  {
    config.isValid = false;
  }

  return config;
}

void ConfigManager::saveSettingsConfig(const String &key, const bool &value)
{
  preferences.begin(SETTINGS_NAMESPACE, false);
  preferences.putBool(key.c_str(), value);
  preferences.end();
}

SettingConfig ConfigManager::loadSettingsConfig()
{
  SettingConfig config;

  preferences.begin(SETTINGS_NAMESPACE, true);
  config.bluetooth = preferences.getBool("bluetooth", false);
  config.wifi = preferences.getBool("wifi", false);

  preferences.end();
  return config;
}

void ConfigManager::clearWiFiConfig()
{
  clearAllWiFiConfigs();
}

String ConfigManager::getDeviceID()
{
  uint64_t chipid = ESP.getEfuseMac();
  char chipID[17];
  sprintf(chipID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(chipID);
}