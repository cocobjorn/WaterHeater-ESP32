#include "config_storage.h"

bool ConfigStorage::isEEPROMInitialized = false;

void ConfigStorage::begin() {
  if (!isEEPROMInitialized) {
    EEPROM.begin(EEPROM_SIZE);
    isEEPROMInitialized = true;
  }
}

bool ConfigStorage::loadConfig(ConfigData& config) {
  begin();
  
  // Читаем данные из EEPROM
  EEPROM.get(CONFIG_ADDRESS, config);
  
  // Проверяем версию и контрольную сумму
  if (config.version != CONFIG_VERSION) {
    return false; // Несовместимая версия
  }
  
  if (!validateConfig(config)) {
    return false; // Неверная контрольная сумма
  }
  
  return true;
}

bool ConfigStorage::saveConfig(const ConfigData& config) {
  begin();
  
  // Проверяем валидность конфигурации
  if (!validateConfig(config)) {
    return false;
  }
  
  // Сохраняем в EEPROM
  EEPROM.put(CONFIG_ADDRESS, config);
  EEPROM.commit();
  
  return true;
}

void ConfigStorage::resetToDefaults(ConfigData& config) {
  // Сбрасываем все значения к умолчанию
  config.version = CONFIG_VERSION;
  
  // Настройки температуры
  config.targetTemp = TARGET_TEMP_DEFAULT;
  config.tempHysteresis = TEMP_HYSTERESIS;
  
  // Настройки протока
  config.flowThreshold = FLOW_THRESHOLD;
  config.flowCalibration = FLOW_CALIBRATION_FACTOR;
  
  // Настройки безопасности
  config.maxTemp = MAX_TEMP;
  config.minTemp = MIN_TEMP;
  config.safetyTimeout = SAFETY_TIMEOUT;
  
  // Настройки нагрева
  config.heatingPower = 100.0f;
  config.heatingDelay = 2000;
  
  // Калибровка температуры
  config.tempOffset = 0.0f;
  config.tempSlope = 1.0f;
  
  // Настройки WiFi
  strncpy(config.wifiSSID, WIFI_SSID, sizeof(config.wifiSSID) - 1);
  strncpy(config.wifiPassword, WIFI_PASSWORD, sizeof(config.wifiPassword) - 1);
  
  // Флаги
  config.isCalibrated = false;
  config.isConfigured = false;
  
  // Пересчитываем контрольную сумму
  config.checksum = calculateChecksum(config);
}

bool ConfigStorage::validateConfig(const ConfigData& config) {
  // Проверяем версию
  if (config.version != CONFIG_VERSION) {
    return false;
  }
  
  // Проверяем контрольную сумму
  uint32_t calculatedChecksum = calculateChecksum(config);
  if (config.checksum != calculatedChecksum) {
    return false;
  }
  
  // Проверяем разумные значения
  if (config.targetTemp < MIN_TEMP || config.targetTemp > MAX_TEMP) {
    return false;
  }
  
  if (config.flowThreshold < 0.1f || config.flowThreshold > 10.0f) {
    return false;
  }
  
  return true;
}

uint32_t ConfigStorage::calculateChecksum(const ConfigData& config) {
  uint32_t checksum = 0;
  const uint8_t* data = (const uint8_t*)&config;
  size_t size = sizeof(ConfigData) - sizeof(config.checksum);
  
  for (size_t i = 0; i < size; i++) {
    checksum += data[i];
  }
  
  return checksum;
}
