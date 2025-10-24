#include "safety.h"
#include "triac_control.h"
#include <Arduino.h>
#include <HardwareSerial.h>

void SafetyManager::begin() {
  pinMode(THERMAL_FUSE_PIN, INPUT_PULLUP);
  
  if (DEBUG_SERIAL) {
    Serial.println("Система безопасности инициализирована");
  }
}

void SafetyManager::checkSafety(SystemState& state) {
  // Проверяем, не истекло ли время аварийного режима
  if (emergencyMode && (millis() - emergencyStartTime) > 5000) {
    emergencyMode = false;
    state.isSystemEnabled = true;
    if (DEBUG_SERIAL) {
      Serial.println("АВАРИЙНЫЙ РЕЖИМ АВТОМАТИЧЕСКИ СБРОШЕН - СИСТЕМА ВОССТАНОВЛЕНА");
    }
  }
  
  // Упрощенная система безопасности (только если не в аварийном режиме)
  if (!emergencyMode) {
    checkBasicSafety(state);
  }
  
  // Обновление состояния системы
  state.isThermalFuseOK = thermalFuseState;
  
  if (!isSystemSafe()) {
    emergencyShutdown(nullptr); // triacControl будет передан из основного цикла
    state.isSystemEnabled = false;
  }
}

void SafetyManager::emergencyShutdown(class TriacControl* triacControl) {
  if (!emergencyMode) {
    emergencyMode = true;
    emergencyStartTime = millis();
    logSafetyEvent("АВАРИЙНОЕ ОТКЛЮЧЕНИЕ СИСТЕМЫ НА 5 СЕКУНД");
    
    // Отключение всех симисторов через ШИМ
    if (triacControl != nullptr) {
      triacControl->emergencyStop();
    }
    
    // Отключение питания реле через MOSFET-модуль
    digitalWrite(MOSFET_EN_PIN, LOW);
    
    if (DEBUG_SERIAL) {
      Serial.println("СИСТЕМА ОТКЛЮЧЕНА ПО ПРИЧИНАМ БЕЗОПАСНОСТИ НА 5 СЕКУНД");
    }
  }
}

bool SafetyManager::isThermalFuseOK() {
  return thermalFuseState;
}

bool SafetyManager::isSystemSafe() {
  return thermalFuseState && !emergencyMode;
}

void SafetyManager::resetEmergencyMode() {
  emergencyMode = false;
  if (DEBUG_SERIAL) {
    Serial.println("АВАРИЙНЫЙ РЕЖИМ СБРОШЕН");
  }
}

void SafetyManager::checkThermalFuse() {
  // Проверка термопредохранителя каждые 100мс
  if (millis() - lastThermalCheck >= 100) {
    int pinValue = digitalRead(THERMAL_FUSE_PIN);
    bool currentState = (pinValue == HIGH); // HIGH = термопредохранитель ОК
    
    // Диагностика каждые 5 секунд
    static unsigned long lastDiagnostic = 0;
    if (millis() - lastDiagnostic >= 5000) {
      Serial.printf("ДИАГНОСТИКА ТЕРМОПРЕДОХРАНИТЕЛЯ: Пин=%d, Состояние=%s\n", 
                    pinValue, currentState ? "ОК" : "СРАБОТАЛ");
      lastDiagnostic = millis();
    }
    
    if (currentState != thermalFuseState) {
      thermalFuseState = currentState;
      if (!thermalFuseState) {
        logSafetyEvent("СРАБОТАЛ ТЕРМОПРЕДОХРАНИТЕЛЬ");
      } else {
        logSafetyEvent("Термопредохранитель восстановлен");
      }
    }
    
    lastThermalCheck = millis();
  }
}

void SafetyManager::checkFlowTimeout(SystemState& state) {
  // МГНОВЕННАЯ проверка аналогового датчика протока (БЕЗОПАСНОСТЬ)
  // Используем более точную аналоговую проверку вместо цифровой
  
  if (state.isHeating && !state.isFlowDetected) {
    String message = "МГНОВЕННОЕ ОТКЛЮЧЕНИЕ - НЕТ ПРОТОКА (АНАЛОГОВЫЙ ДАТЧИК: " + String(state.flowRate, 2) + " л/мин)";
    logSafetyEvent(message.c_str());
    state.isHeating = false;
  }
}

void SafetyManager::checkTemperatureLimits(SystemState& state) {
  // Проверка максимальной температуры
  if (state.currentTemp > MAX_TEMP_SAFETY) {
    logSafetyEvent("ПРЕВЫШЕНА МАКСИМАЛЬНАЯ ТЕМПЕРАТУРА");
    emergencyShutdown(nullptr);
  }
  
  // ДОПОЛНИТЕЛЬНАЯ БЕЗОПАСНОСТЬ: Проверка критически высокой температуры
  if (state.currentTemp > 75.0) {
    logSafetyEvent("КРИТИЧЕСКИ ВЫСОКАЯ ТЕМПЕРАТУРА - ПРЕДУПРЕЖДЕНИЕ");
    // Принудительно снижаем мощность
    if (state.isHeating) {
      state.heatingPower[0] = min(state.heatingPower[0], (uint8_t)30);
      state.heatingPower[1] = min(state.heatingPower[1], (uint8_t)30);
      state.heatingPower[2] = min(state.heatingPower[2], (uint8_t)30);
    }
  }
  
  // Проверка на заклинивание датчика температуры
  static float lastTemp = 0;
  static unsigned long lastTempChange = 0;
  
  if (abs(state.currentTemp - lastTemp) > 0.1) {
    lastTemp = state.currentTemp;
    lastTempChange = millis();
  }
  
  if ((millis() - lastTempChange) > 30000 && state.isHeating) {
    logSafetyEvent("ОШИБКА ДАТЧИКА ТЕМПЕРАТУРЫ");
    emergencyShutdown(nullptr);
  }
  
  // Для проточного водонагревателя быстрый рост температуры - это нормально
  // Убираем ограничение на быстрый рост, так как это основная задача системы
}

void SafetyManager::checkPowerLimits(SystemState& state) {
  // Проверка каждые 500мс
  if (millis() - lastPowerCheck < 500) return;
  lastPowerCheck = millis();
  
  // Проверка на слишком высокую мощность при малом протоке
  if (state.flowRate < FLOW_THRESHOLD_MIN && state.isHeating) {
    uint8_t totalPower = state.heatingPower[0] + state.heatingPower[1] + state.heatingPower[2];
    if (totalPower > MAX_POWER_AT_LOW_FLOW * 3) { // Более лимита на все фазы при малом протоке
      logSafetyEvent("ПРЕВЫШЕНА МОЩНОСТЬ ПРИ МАЛОМ ПРОТОКЕ");
      emergencyShutdown(nullptr);
    }
  }
  
  // Проверка на асимметрию мощности между фазами
  uint8_t maxPower = max(state.heatingPower[0], max(state.heatingPower[1], state.heatingPower[2]));
  uint8_t minPower = min(state.heatingPower[0], min(state.heatingPower[1], state.heatingPower[2]));
  if (maxPower - minPower > 20) { // Разница более 20% между фазами
    logSafetyEvent("АСИММЕТРИЯ МОЩНОСТИ МЕЖДУ ФАЗАМИ");
    // Не отключаем, но предупреждаем
  }
  
  // Проверка на заклинивание в режиме разгона
  if (state.isPowerRamping && (millis() - state.heatingStartTime) > 10000) {
    logSafetyEvent("ЗАКЛИНИВАНИЕ В РЕЖИМЕ РАЗГОНА");
    state.isPowerRamping = false;
  }
}

void SafetyManager::checkBasicSafety(SystemState& state) {
  // 1. Проверка термопредохранителя
  checkThermalFuse();
  
  // 2. Проверка диапазона температуры (аварийные значения)
  if (state.currentTemp > 70.0) {
    logSafetyEvent("ПРЕВЫШЕНИЕ АВАРИЙНОЙ ТЕМПЕРАТУРЫ 70°C");
    emergencyShutdown(nullptr);
    return;
  }
  
  // 3. Проверка только минимального порога протока (0.5 л/мин)
  if (state.flowRate > 0.0 && state.flowRate < 0.5) {
    String message = "СЛИШКОМ МАЛЫЙ ПРОТОК: " + String(state.flowRate, 2) + " л/мин";
    logSafetyEvent(message.c_str());
    emergencyShutdown(nullptr);
    return;
  }
  
  // 4. Проверка на зависание показаний (2 секунды одинаковые значения)
  static float lastTemp = -999;
  static float lastFlow = -999;
  static unsigned long lastChange = 0;
  
  unsigned long currentTime = millis();
  
  if (state.currentTemp != lastTemp || state.flowRate != lastFlow) {
    lastTemp = state.currentTemp;
    lastFlow = state.flowRate;
    lastChange = currentTime;
  } else if (currentTime - lastChange > 2000) {
    logSafetyEvent("ЗАВИСАНИЕ ДАТЧИКОВ - ОДИНАКОВЫЕ ПОКАЗАНИЯ 2 СЕК");
    emergencyShutdown(nullptr);
    return;
  }
}

void SafetyManager::checkSystemIntegrity(SystemState& state) {
  // Если нет протока - отключаем все проверки безопасности
  if (!state.isFlowDetected) {
    return; // Выходим без проверок в режиме сна
  }
  
  // Проверка на зависание системы (только при активной системе)
  static unsigned long lastSystemCheck = 0;
  static unsigned long lastLoopTime = 0;
  
  unsigned long currentTime = millis();
  
  // Обновляем время последнего цикла
  lastLoopTime = currentTime;
  
  if (currentTime - lastSystemCheck > 10000) { // Каждые 10 секунд при активной системе
    if (currentTime - lastLoopTime > SYSTEM_WATCHDOG_TIMEOUT_MS) {
      logSafetyEvent("ЗАВИСАНИЕ СИСТЕМЫ - ПЕРЕЗАГРУЗКА");
      emergencyShutdown(nullptr);
      // Здесь можно добавить перезагрузку ESP32
      // ESP.restart();
    }
    lastSystemCheck = currentTime;
  }
  
  // Проверка на некорректные значения температуры (только если система активна)
  if (state.currentTemp < TEMP_RANGE_MIN || state.currentTemp > TEMP_RANGE_MAX) {
    logSafetyEvent("НЕКОРРЕКТНОЕ ЗНАЧЕНИЕ ТЕМПЕРАТУРЫ");
    emergencyShutdown(nullptr);
  }
  
  // Проверка на некорректные значения протока (только если система активна)
  if (state.flowRate < 0.0 || state.flowRate > FLOW_RANGE_MAX) {
    logSafetyEvent("НЕКОРРЕКТНОЕ ЗНАЧЕНИЕ ПРОТОКА");
    emergencyShutdown(nullptr);
  }
}

void SafetyManager::logSafetyEvent(const char* message) {
  if (DEBUG_SERIAL) {
    static unsigned long lastLogTime = 0;
    static const char* lastMessage = "";
    
    // Логируем не чаще раза в 5 секунд для одинаковых сообщений
    if (millis() - lastLogTime > 5000 || strcmp(message, lastMessage) != 0) {
      Serial.print("[БЕЗОПАСНОСТЬ] ");
      Serial.print(millis());
      Serial.print(": ");
      Serial.println(message);
      lastLogTime = millis();
      lastMessage = message;
    }
  }
}
