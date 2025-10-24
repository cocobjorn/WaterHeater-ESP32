#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <EEPROM.h>
#include <ArduinoJson.h>
#include "config.h"

// Структура для сохранения конфигурации в EEPROM
struct ConfigData {
  // Версия конфигурации для совместимости
  uint32_t version = 1;
  
  // Настройки температуры
  float targetTemp;
  float tempHysteresis;
  
  // Настройки протока
  float flowThreshold;
  float flowCalibration;
  
  // Настройки безопасности
  float maxTemp;
  float minTemp;
  unsigned long safetyTimeout;
  
  // Настройки нагрева
  float heatingPower;
  unsigned long heatingDelay;
  
  // Калибровка температуры
  float tempOffset;
  float tempSlope;
  
  // Настройки WiFi
  char wifiSSID[32];
  char wifiPassword[64];
  
  // Флаги
  bool isCalibrated;
  bool isConfigured;
  
  // Контрольная сумма
  uint32_t checksum;
};

// Константы для EEPROM
#define CONFIG_VERSION 1
#define EEPROM_SIZE 512
#define CONFIG_ADDRESS 0

// Функции для работы с конфигурацией
class ConfigStorage {
public:
  static void begin();
  static bool loadConfig(ConfigData& config);
  static bool saveConfig(const ConfigData& config);
  static void resetToDefaults(ConfigData& config);
  static bool validateConfig(const ConfigData& config);
  static uint32_t calculateChecksum(const ConfigData& config);
  
private:
  static bool isEEPROMInitialized;
};

#endif
