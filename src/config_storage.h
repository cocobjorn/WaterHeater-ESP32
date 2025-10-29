#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <EEPROM.h>
#include "config.h"
#include "system_state.h"

class ConfigStorage {
public:
    // Инициализация EEPROM
    static void begin();
    
    // Сохранение конфигурации
    static bool saveConfig(const SystemState& state);
    
    // Загрузка конфигурации
    static bool loadConfig(SystemState& state);
    
    // Сброс к настройкам по умолчанию
    static void resetToDefaults(SystemState& state);
    
    // Проверка валидности данных в EEPROM
    static bool isValidConfig();
    
private:
    // Адреса в EEPROM
    static const int EEPROM_SIZE = 512;
    static const int MAGIC_NUMBER_ADDR = 0;
    static const int CONFIG_START_ADDR = 4;
    
    // Магическое число для проверки валидности
    static const uint32_t MAGIC_NUMBER;
    
    // Структура конфигурации для EEPROM
    struct EEPROMConfig {
        float targetTemp;
        float flowCalibrationFactor;
        uint32_t magic;
    };
};

#endif
