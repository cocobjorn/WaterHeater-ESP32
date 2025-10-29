#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "config.h"

// Структура состояния системы для веб-интерфейса
struct SystemState {
    // Основные параметры
    float currentTemp;              // Текущая температура (°C)
    float targetTemp;               // Целевая температура (°C)
    float flowRate;                 // Скорость потока (л/мин)
    float flowCalibrationFactor;    // Коэффициент калибровки потока
    
    // Состояние системы
    bool isHeating;                 // Нагрев включен
    bool isFlowDetected;           // Обнаружен поток
    bool isThermalFuseOK;           // Термопредохранитель в порядке
    bool isSystemEnabled;           // Система включена
    bool isPowerRamping;            // Плавный разгон мощности
    
    // Мощность по фазам
    float heatingPower[3];          // Текущая мощность по фазам (0-100%)
    float targetPower[3];           // Целевая мощность по фазам (0-100%)
    float currentTargetPower;       // Текущая целевая мощность
    
    // Режим работы системы
    int systemMode;                 // Режим работы (SYSTEM_MODE_*)
    bool isWiFiEnabled;            // WiFi включен
    unsigned long wifiSessionStartTime; // Время начала WiFi сессии
    
    // Методы управления конфигурацией
    bool saveConfiguration();
    bool loadConfiguration();
    void resetConfiguration();
    
    // Конструктор с инициализацией по умолчанию
    SystemState() {
        currentTemp = 25.0;
        targetTemp = TARGET_TEMP_DEFAULT;
        flowRate = 0.0;
        flowCalibrationFactor = FLOW_CALIBRATION_FACTOR;
        
        isHeating = false;
        isFlowDetected = false;
        isThermalFuseOK = true;
        isSystemEnabled = true;
        isPowerRamping = false;
        
        for (int i = 0; i < 3; i++) {
            heatingPower[i] = 0.0;
            targetPower[i] = 0.0;
        }
        
        currentTargetPower = 0.0;
        
        systemMode = SYSTEM_MODE_ACTIVE;
        isWiFiEnabled = false;
        wifiSessionStartTime = 0;
    }
};

#endif
