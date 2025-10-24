#include "system_state.h"
#include "config_storage.h"

bool SystemState::loadConfiguration() {
  // Пытаемся загрузить конфигурацию из EEPROM
  if (ConfigStorage::loadConfig(config)) {
    // Применяем загруженные настройки
    targetTemp = config.targetTemp;
    flowCalibrationFactor = config.flowCalibration;
    
    // Применяем калибровку температуры
    if (config.isCalibrated) {
      // Здесь нужно будет добавить функцию установки калибровки
      // setTemperatureCalibration(config.tempOffset, config.tempSlope);
    }
    
    if (DEBUG_SERIAL) {
      Serial.println("=== КОНФИГУРАЦИЯ ЗАГРУЖЕНА ИЗ EEPROM ===");
      Serial.printf("Версия конфигурации: %d\n", config.version);
      Serial.printf("Целевая температура: %.1f°C\n", targetTemp);
      Serial.printf("Калибровка протока: %.2f\n", flowCalibrationFactor);
      Serial.printf("Порог протока: %.2f л/мин\n", config.flowThreshold);
      Serial.printf("Калибровка температуры: %s\n", config.isCalibrated ? "ДА" : "НЕТ");
      Serial.printf("Контрольная сумма: 0x%08X\n", config.checksum);
      Serial.println("========================================");
    }
    
    return true;
  } else {
    // Если загрузка не удалась - сбрасываем к умолчанию
    if (DEBUG_SERIAL) {
      Serial.println("Ошибка загрузки конфигурации - используем настройки по умолчанию");
    }
    resetConfiguration();
    return false;
  }
}

bool SystemState::saveConfiguration() {
  // Обновляем конфигурацию текущими значениями
  config.targetTemp = targetTemp;
  config.flowCalibration = flowCalibrationFactor;
  config.flowThreshold = FLOW_THRESHOLD; // Устанавливаем обязательное поле
  config.version = CONFIG_VERSION; // Устанавливаем версию
  
  // Получаем текущую калибровку температуры
  // Здесь нужно будет добавить функцию получения калибровки
  // float offset, slope;
  // getTemperatureCalibration(offset, slope);
  config.tempOffset = 0.0f; // offset;
  config.tempSlope = 1.0f;  // slope;
  config.isCalibrated = false; // (offset != 0.0f || slope != 1.0f);
  
  // Пересчитываем контрольную сумму
  config.checksum = ConfigStorage::calculateChecksum(config);
  
  // Сохраняем в EEPROM
  if (ConfigStorage::saveConfig(config)) {
    if (DEBUG_SERIAL) {
      Serial.println("=== КОНФИГУРАЦИЯ СОХРАНЕНА В EEPROM ===");
      Serial.printf("Целевая температура: %.1f°C\n", targetTemp);
      Serial.printf("Калибровка протока: %.2f\n", flowCalibrationFactor);
      Serial.printf("Порог протока: %.2f л/мин\n", config.flowThreshold);
      Serial.printf("Контрольная сумма: 0x%08X\n", config.checksum);
      Serial.println("========================================");
    }
    return true;
  } else {
    if (DEBUG_SERIAL) {
      Serial.println("Ошибка сохранения конфигурации в EEPROM");
    }
    return false;
  }
}

void SystemState::resetConfiguration() {
  // Сбрасываем к значениям по умолчанию
  ConfigStorage::resetToDefaults(config);
  
  // Применяем настройки по умолчанию
  targetTemp = config.targetTemp;
  flowCalibrationFactor = config.flowCalibration;
  
  // Сбрасываем калибровку температуры
  // Здесь нужно будет добавить функцию установки калибровки
  // setTemperatureCalibration(0.0f, 1.0f);
  config.isCalibrated = false;
  
  // Сохраняем сброшенную конфигурацию
  saveConfiguration();
  
  if (DEBUG_SERIAL) {
    Serial.println("=== КОНФИГУРАЦИЯ СБРОШЕНА К ЗНАЧЕНИЯМ ПО УМОЛЧАНИЮ ===");
    Serial.printf("Целевая температура: %.1f°C\n", targetTemp);
    Serial.printf("Калибровка протока: %.2f\n", flowCalibrationFactor);
    Serial.printf("Порог протока: %.2f л/мин\n", config.flowThreshold);
    Serial.println("=====================================================");
  }
}

bool SystemState::calibrateTemperature(float referenceTemp) {
  // Получаем текущую сырую температуру
  float rawTemp = currentTemp;
  
  // Рассчитываем смещение (offset) для калибровки
  config.tempOffset = referenceTemp - rawTemp;
  config.tempSlope = 1.0f; // Пока используем только смещение
  config.isCalibrated = true;
  
  // Сохраняем калибровку в EEPROM
  if (saveConfiguration()) {
    if (DEBUG_SERIAL) {
      Serial.println("=== КАЛИБРОВКА ТЕМПЕРАТУРЫ ВЫПОЛНЕНА ===");
      Serial.printf("Сырая температура: %.1f°C\n", rawTemp);
      Serial.printf("Эталонная температура: %.1f°C\n", referenceTemp);
      Serial.printf("Смещение: %.1f°C\n", config.tempOffset);
      Serial.printf("Коэффициент: %.2f\n", config.tempSlope);
      Serial.println("=====================================");
    }
    return true;
  } else {
    if (DEBUG_SERIAL) {
      Serial.println("Ошибка сохранения калибровки температуры");
    }
    return false;
  }
}

float SystemState::getCalibratedTemperature(float rawTemp) {
  if (!config.isCalibrated) {
    return rawTemp; // Если не калибровано - возвращаем сырую температуру
  }
  
  // Применяем калибровку: temp_calibrated = (temp_raw + offset) * slope
  float calibratedTemp = (rawTemp + config.tempOffset) * config.tempSlope;
  
  return calibratedTemp;
}

void SystemState::resetTemperatureCalibration() {
  config.tempOffset = 0.0f;
  config.tempSlope = 1.0f;
  config.isCalibrated = false;
  
  // Сохраняем сброс калибровки
  saveConfiguration();
  
  if (DEBUG_SERIAL) {
    Serial.println("=== КАЛИБРОВКА ТЕМПЕРАТУРЫ СБРОШЕНА ===");
    Serial.println("Используется сырая температура без калибровки");
    Serial.println("=======================================");
  }
}

bool SystemState::isTemperatureCalibrated() {
  return config.isCalibrated;
}
