#include "config_storage.h"

// Определение статических констант
const uint32_t ConfigStorage::MAGIC_NUMBER = 0x57415445; // "WATE" (Water Heater)

void ConfigStorage::begin() {
    EEPROM.begin(EEPROM_SIZE);
}

bool ConfigStorage::saveConfig(const SystemState& state) {
    EEPROMConfig config;
    config.targetTemp = state.targetTemp;
    config.flowCalibrationFactor = state.flowCalibrationFactor;
    config.magic = MAGIC_NUMBER;
    
    // Записываем магическое число
    EEPROM.put(MAGIC_NUMBER_ADDR, MAGIC_NUMBER);
    
    // Записываем конфигурацию
    EEPROM.put(CONFIG_START_ADDR, config);
    
    // Сохраняем изменения
    bool result = EEPROM.commit();
    
    if (DEBUG_SERIAL) {
        Serial.println("Конфигурация сохранена в EEPROM: " + String(result ? "УСПЕХ" : "ОШИБКА"));
        Serial.println("Целевая температура: " + String(config.targetTemp, 1) + "°C");
        Serial.println("Коэффициент калибровки: " + String(config.flowCalibrationFactor, 2) + " имп/л");
    }
    
    return result;
}

bool ConfigStorage::loadConfig(SystemState& state) {
    uint32_t magic;
    EEPROM.get(MAGIC_NUMBER_ADDR, magic);
    
    if (magic != MAGIC_NUMBER) {
        if (DEBUG_SERIAL) {
            Serial.println("EEPROM не содержит валидной конфигурации, используем настройки по умолчанию");
        }
        return false;
    }
    
    EEPROMConfig config;
    EEPROM.get(CONFIG_START_ADDR, config);
    
    // Проверяем валидность данных
    if (config.targetTemp < TARGET_TEMP_MIN || config.targetTemp > TARGET_TEMP_MAX ||
        config.flowCalibrationFactor < FLOW_CALIBRATION_MIN || config.flowCalibrationFactor > FLOW_CALIBRATION_MAX) {
        if (DEBUG_SERIAL) {
            Serial.println("Конфигурация в EEPROM содержит невалидные данные, используем настройки по умолчанию");
        }
        return false;
    }
    
    // Загружаем валидные данные
    state.targetTemp = config.targetTemp;
    state.flowCalibrationFactor = config.flowCalibrationFactor;
    
    if (DEBUG_SERIAL) {
        Serial.println("Конфигурация загружена из EEPROM:");
        Serial.println("Целевая температура: " + String(state.targetTemp, 1) + "°C");
        Serial.println("Коэффициент калибровки: " + String(state.flowCalibrationFactor, 2) + " имп/л");
    }
    
    return true;
}

void ConfigStorage::resetToDefaults(SystemState& state) {
    state.targetTemp = TARGET_TEMP_DEFAULT;
    state.flowCalibrationFactor = FLOW_CALIBRATION_FACTOR;
    
    // Очищаем EEPROM
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    
    if (DEBUG_SERIAL) {
        Serial.println("Конфигурация сброшена к настройкам по умолчанию");
    }
}

bool ConfigStorage::isValidConfig() {
    uint32_t magic;
    EEPROM.get(MAGIC_NUMBER_ADDR, magic);
    return magic == MAGIC_NUMBER;
}
