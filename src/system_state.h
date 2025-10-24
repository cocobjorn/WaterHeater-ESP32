#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>
#include "config_storage.h"

struct SystemState {
  float currentTemp = 0.0;
  float targetTemp = 50.0;
  float flowRate = 0.0;
  float flowCalibrationFactor = 7.5;  // Коэффициент калибровки датчика протока
  bool isHeating = false;
  bool isFlowDetected = false;
  bool isThermalFuseOK = true;
  bool isSystemEnabled = false;
  uint8_t heatingPower[3] = {0, 0, 0};
  uint8_t targetPower[3] = {0, 0, 0};  // Целевая мощность для плавного выхода
  unsigned long lastFlowTime = 0;
  unsigned long lastSafetyCheck = 0;
  unsigned long heatingStartTime = 0;
  unsigned long lastPowerRampTime = 0;  // Время последнего изменения мощности
  bool isPowerRamping = false;  // Флаг плавного изменения мощности
  
  // Новые поля для архитектуры сна
  uint8_t systemMode = 0;  // SYSTEM_MODE_SLEEP, SYSTEM_MODE_ACTIVE, SYSTEM_MODE_WIFI_SESSION
  bool isWiFiEnabled = false;
  unsigned long wifiSessionStartTime = 0;
  unsigned long lastButtonPress = 0;
  uint8_t buttonPressCount = 0;
  bool isInDeepSleep = false;
  
  // Конфигурация из EEPROM
  ConfigData config;
  
  // Методы для работы с конфигурацией
  bool loadConfiguration();
  bool saveConfiguration();
  void resetConfiguration();
  
  // Методы калибровки температуры
  bool calibrateTemperature(float referenceTemp);
  float getCalibratedTemperature(float rawTemp);
  void resetTemperatureCalibration();
  bool isTemperatureCalibrated();
};

#endif
