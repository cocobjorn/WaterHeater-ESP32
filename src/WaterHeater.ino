#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include "esp_bt.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "system_state.h"
#include "sensors.h"
#include "triac_control.h"
#include "web_server.h"
#include "safety.h"
#include "utils.h"
#include "terminal_commands.h"

// Глобальные переменные состояния
SystemState systemState;

// Объекты для управления
Sensors sensors;
TriacControl triacControl;
SafetyManager safetyManager;
WebServerManager webServer;

// Таймеры
unsigned long lastSensorRead = 0;
unsigned long lastWebUpdate = 0;
unsigned long lastSerialMonitor = 0;

// Флаг состояния CPU
bool cpuOptimizedForIdle = false;

void setup() {
  // Устанавливаем оптимальную частоту CPU
  setCpuFrequencyMhz(240);
  
  // Отключаем Bluetooth для экономии энергии
  esp_bt_controller_disable();
  
  Serial.begin(115200);
  
  if (DEBUG_SERIAL) {
    Serial.println("=== Проточный водонагреватель (Sleep Architecture) ===");
    Serial.println("Инициализация системы...");
    Serial.printf("CPU частота: %d MHz\n", getCpuFrequencyMhz());
  }
  
  // Инициализация EEPROM
  EEPROM.begin(512);
  
  // Инициализация компонентов
  initializePins();
  sensors.begin();
  triacControl.begin();
  safetyManager.begin();
  
  // Настройка кнопки BOOT
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  
  // Настройка светодиода индикации
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW); // Выключаем светодиод по умолчанию
  
  // Загрузка конфигурации из EEPROM
  if (!systemState.loadConfiguration()) {
    if (DEBUG_SERIAL) {
      Serial.println("Используются настройки по умолчанию");
    }
  }
  
  // Включаем систему после загрузки конфигурации
  systemState.isSystemEnabled = true;
  
  if (DEBUG_SERIAL) {
    Serial.printf("Система включена. Целевая температура: %.1f°C\n", systemState.targetTemp);
  }
  
  // Инициализация терминала
  TerminalManager::begin();
  
  // Добавляем системные логи в терминал
  TerminalManager::addLog("=== СИСТЕМА ИНИЦИАЛИЗИРОВАНА ===");
  TerminalManager::addLog("CPU частота: " + String(getCpuFrequencyMhz()) + " MHz");
  TerminalManager::addLog("Свободная память: " + String(ESP.getFreeHeap()) + " байт");
  TerminalManager::addLog("Размер стека: " + String(uxTaskGetStackHighWaterMark(NULL)) + " байт");
  TerminalManager::addLog("Температура чипа: " + String(temperatureRead(), 1) + "°C");
  
  // Проверяем состояние датчика протока при запуске
  int flowSensorState = digitalRead(FLOW_SENSOR_PIN);
  if (DEBUG_SERIAL) {
    Serial.printf("Состояние датчика протока при запуске: %d\n", flowSensorState);
  }
  
  // Определяем начальный режим работы
  if (flowSensorState == LOW) {
    // Датчик протока активен - переходим в активный режим
    if (DEBUG_SERIAL) {
      Serial.println("Обнаружен проток при запуске - переход в активный режим");
    }
    TerminalManager::addLog("Обнаружен проток при запуске");
    systemState.systemMode = SYSTEM_MODE_ACTIVE;
    systemState.isFlowDetected = true;
  } else {
    // Нет протока - режим ожидания
    if (DEBUG_SERIAL) {
      Serial.println("Проток не обнаружен - режим ожидания");
    }
    systemState.systemMode = SYSTEM_MODE_SLEEP;
  }
  
  if (DEBUG_SERIAL) {
    Serial.println("Система инициализирована успешно");
    Serial.printf("Режим работы: %d\n", systemState.systemMode);
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Обработка кнопки BOOT (только если WiFi не активен)
  if (!systemState.isWiFiEnabled) {
    checkBootButton();
  }
  
  // Проверка таймаута WiFi сессии
  if (systemState.isWiFiEnabled) {
    unsigned long sessionDuration = currentTime - systemState.wifiSessionStartTime;
    
    // Отладка только при запуске WiFi сессии
    static bool debugLogged = false;
    static unsigned long lastWifiStart = 0;
    
    // Сбрасываем флаг отладки при новом запуске WiFi
    if (systemState.wifiSessionStartTime != lastWifiStart) {
      debugLogged = false;
      lastWifiStart = systemState.wifiSessionStartTime;
    }
    
    if (!debugLogged && sessionDuration < 1000) { // Только первые 1000мс
      if (DEBUG_SERIAL) {
        Serial.printf("DEBUG: currentTime=%lu, wifiStart=%lu, duration=%lu\n", 
                     currentTime, systemState.wifiSessionStartTime, sessionDuration);
      }
      debugLogged = true;
    }
    
    if (sessionDuration > WIFI_SESSION_TIMEOUT_MS) {
      disableWiFiSession();
    }
  }
  
  switch(systemState.systemMode) {
    case SYSTEM_MODE_SLEEP:
      handleSleepMode();
      break;
      
    case SYSTEM_MODE_ACTIVE:
      handleActiveMode(currentTime);
      break;
      
    case SYSTEM_MODE_WIFI_SESSION:
      handleWiFiSessionMode(currentTime);
      break;
  }
}

void initializePins() {
  // Настройка пинов симисторов (управляются через ШИМ, не через digitalWrite)
  // pinMode не нужен для ШИМ пинов - они настраиваются в TriacControl::begin()
  
  // Управление модулем MOSFET, который подает 5В на блок реле
  pinMode(MOSFET_EN_PIN, OUTPUT);
  
  // Настройка пина термопредохранителя
  pinMode(THERMAL_FUSE_PIN, INPUT_PULLUP);
  
  // Настройка пина датчика протока
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  
  // Настройка пина детектора нулевого перехода (H11AA1)
  // Используем INPUT без подтяжки, так как H11AA1 имеет встроенный pull-up
  pinMode(ZERO_CROSS_PIN, INPUT);
  
  // Изначально все выключено (симисторы управляются через ШИМ)
  // digitalWrite не работает для ШИМ пинов
  digitalWrite(MOSFET_EN_PIN, LOW);   // По умолчанию питание реле отключено
}

void readSensors() {
  // Чтение сырой температуры
  float rawTemp = sensors.readTemperature();
  
  // Применяем калибровку температуры
  systemState.currentTemp = systemState.getCalibratedTemperature(rawTemp);
  
  // Чтение протока
  systemState.flowRate = sensors.readFlowRate();
  systemState.isFlowDetected = (systemState.flowRate >= FLOW_THRESHOLD_MIN);
  
  if (systemState.isFlowDetected) {
    systemState.lastFlowTime = millis();
  }
  
  // Проверка термопредохранителя
  systemState.isThermalFuseOK = digitalRead(THERMAL_FUSE_PIN) == HIGH;
}

void serialMonitor() {
  // Мониторинг температуры и протока каждую секунду
  unsigned long currentTime = millis();
  
  if (currentTime - lastSerialMonitor >= SERIAL_MONITOR_INTERVAL) {
    // Используем уже прочитанные значения из systemState для экономии времени
    int flowSensorValue = digitalRead(FLOW_SENSOR_PIN);
    int zeroCrossValue = triacControl.getZeroCrossRawValue();
    bool zeroCrossState = triacControl.readZeroCrossState();
    
    // Дополнительная диагностика датчика протока
    static int lastFlowSensorValue = -1;
    static int flowSensorChanges = 0;
    
    // Диагностика детектора перехода через 0
    static int lastZeroCrossValue = -1;
    static int zeroCrossChanges = 0;
    
    if (flowSensorValue != lastFlowSensorValue) {
      flowSensorChanges++;
      lastFlowSensorValue = flowSensorValue;
    }
    
    if (zeroCrossValue != lastZeroCrossValue) {
      zeroCrossChanges++;
      lastZeroCrossValue = zeroCrossValue;
    }
    
    Serial.printf("МОНИТОРИНГ: Температура=%.1f°C, Проток=%.2f л/мин, Датчик протока=%d, Детектор нуля=%d (состояние=%s), Изменения протока=%d, Изменения нуля=%d, Нагрев=%s\n", 
                  systemState.currentTemp, systemState.flowRate, flowSensorValue, zeroCrossValue, 
                  zeroCrossState ? "ВКЛ 220В" : "ВЫКЛ 220В", flowSensorChanges, zeroCrossChanges, 
                  systemState.isHeating ? "ВКЛ" : "ВЫКЛ");
    
    // Сбрасываем счетчики изменений каждые 10 секунд
    if (currentTime % 10000 < 1000) {
      flowSensorChanges = 0;
      zeroCrossChanges = 0;
    }
    
    lastSerialMonitor = currentTime;
  }
}

void readFlowOnly() {
  // Читаем только проток для режима сна - максимально быстро и эффективно
  systemState.flowRate = sensors.readFlowRate();
  systemState.isFlowDetected = (systemState.flowRate >= FLOW_THRESHOLD_MIN);
  
  if (systemState.isFlowDetected) {
    systemState.lastFlowTime = millis();
  }
}

void monitorZeroCross() {
  static unsigned long lastZeroCrossTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastZeroCrossTime >= 100) {
    lastZeroCrossTime = currentTime;
  }
}

void controlHeating() {
  // Проверка условий для включения нагрева
  bool shouldHeat = systemState.isFlowDetected && 
                   systemState.currentTemp < (systemState.targetTemp - TEMP_HYSTERESIS) &&
                   systemState.isThermalFuseOK &&
                   systemState.isSystemEnabled;
  
  // Диагностика условий нагрева каждые 5 секунд
  static unsigned long lastDiagnostic = 0;
  if (millis() - lastDiagnostic >= 5000) {
    Serial.printf("ДИАГНОСТИКА НАГРЕВА:\n");
    Serial.printf("  Проток обнаружен: %s (%.2f л/мин)\n", 
                  systemState.isFlowDetected ? "ДА" : "НЕТ", systemState.flowRate);
    Serial.printf("  Температура: %.1f°C < %.1f°C (цель-гистерезис): %s\n", 
                  systemState.currentTemp, (systemState.targetTemp - TEMP_HYSTERESIS),
                  (systemState.currentTemp < (systemState.targetTemp - TEMP_HYSTERESIS)) ? "ДА" : "НЕТ");
    Serial.printf("  Термопредохранитель: %s\n", systemState.isThermalFuseOK ? "ОК" : "ОШИБКА");
    Serial.printf("  Система включена: %s\n", systemState.isSystemEnabled ? "ДА" : "НЕТ");
    Serial.printf("  Целевая температура: %.1f°C\n", systemState.targetTemp);
    Serial.printf("  Должен нагревать: %s\n", shouldHeat ? "ДА" : "НЕТ");
    Serial.printf("  Сейчас нагревает: %s\n", systemState.isHeating ? "ДА" : "НЕТ");
    Serial.printf("  Цифровой датчик протока: %d (HIGH=есть проток)\n", digitalRead(FLOW_SENSOR_PIN));
    Serial.printf("  Импульсы датчика: %lu\n", sensors.getFlowPulseCount());
    lastDiagnostic = millis();
  }
  
  if (shouldHeat && !systemState.isHeating) {
    startHeating();
  } else if (!shouldHeat && systemState.isHeating) {
    stopHeating();
  }
  
  // Управление мощностью при активном нагреве
  if (systemState.isHeating) {
    calculateHeatingPower();
    // Используем плавный выход на мощность только если не в режиме разгона
    if (!systemState.isPowerRamping) {
      triacControl.smoothRampToPower(systemState.heatingPower, systemState);
    }
  }
  
  // Обновление плавного изменения мощности
  triacControl.updateRamp(systemState);
  
  // МГНОВЕННАЯ остановка при отсутствии протока (БЕЗОПАСНОСТЬ)
  // Используем аналоговую проверку протока для более точного определения
  if (systemState.isHeating && !systemState.isFlowDetected) {
    if (DEBUG_SERIAL) {
      Serial.printf("МГНОВЕННОЕ отключение - нет протока (аналоговый датчик=%.2f л/мин)\n", systemState.flowRate);
    }
    stopHeating();
  }
}

void startHeating() {
  if (DEBUG_SERIAL) {
    Serial.println("Запуск нагрева - ПЛАВНЫЙ РАЗГОН");
  }
  
  systemState.isHeating = true;
  systemState.heatingStartTime = millis();
  
  // Включение реле с задержкой для безопасности (обратная логика)
  delay(RELAY_SAFETY_DELAY);
  // Включаем питание реле через MOSFET-модуль
  digitalWrite(MOSFET_EN_PIN, HIGH);
  
  // Инициализация плавного разгона
  // Начинаем с 0% мощности - система плавно разгонится до целевой
  for (int i = 0; i < 3; i++) {
    systemState.heatingPower[i] = 0; // Начальная мощность 0%
    systemState.targetPower[i] = 0; // Будет установлено алгоритмом управления
  }
}

void stopHeating() {
  if (DEBUG_SERIAL) {
    Serial.println("Остановка нагрева");
  }
  
  systemState.isHeating = false;
  systemState.isPowerRamping = false;
  
  // Плановая остановка симисторов
  triacControl.normalStop();
  // Отключаем питание реле
  digitalWrite(MOSFET_EN_PIN, LOW);
}

void calculateHeatingPower() {
  // Алгоритм Брезенхэма для плавного управления мощностью
  float tempError = systemState.targetTemp - systemState.currentTemp;
  float flowFactor = constrain(systemState.flowRate / FLOW_THRESHOLD_MAX, 0.5, 1.0);
  
  // Адаптивный гистерезис на основе протока (БЕЗОПАСНОСТЬ)
  float adaptiveHysteresis = TEMP_HYSTERESIS;
  if (systemState.flowRate < FLOW_THRESHOLD_MIN * 2) {
    // Увеличиваем гистерезис при малом протоке для компенсации задержки
    // НО ограничиваем максимум 5°C для безопасности
    adaptiveHysteresis = min(TEMP_HYSTERESIS * (1.0 + (FLOW_THRESHOLD_MIN * 2 - systemState.flowRate) / FLOW_THRESHOLD_MIN), 5.0);
  }
  
  // ДОПОЛНИТЕЛЬНАЯ БЕЗОПАСНОСТЬ: При очень малом протоке увеличиваем гистерезис еще больше
  if (systemState.flowRate < FLOW_THRESHOLD_MIN) {
    adaptiveHysteresis = max(adaptiveHysteresis, 4.0f); // Минимум 4°C при критически малом протоке
  }
  
  // Корректируем ошибку температуры с учетом адаптивного гистерезиса
  if (abs(tempError) < adaptiveHysteresis) {
    tempError = 0; // В зоне гистерезиса - не меняем мощность
  }
  
  // БЕЗОПАСНОСТЬ: Ограничиваем максимальное изменение мощности за один цикл
  static uint8_t lastPower = 0;
  
  // Базовая мощность на основе температуры (улучшенная формула)
  // При ошибке 30°C -> 30*2 = 60% мощности, при 50°C -> 50*2 = 100%
  uint8_t adjustedPower = constrain(map(abs(tempError) * 2, 0, 50, MIN_POWER_PERCENT, MAX_POWER_PERCENT), 
                                   MIN_POWER_PERCENT, MAX_POWER_PERCENT);
  
  // Корректировка на основе протока
  adjustedPower = (uint8_t)(adjustedPower * flowFactor);
  
  // БЕЗОПАСНОСТЬ: Ограничение мощности при малом протоке
  if (systemState.flowRate < 2.0) { // При протоке менее 2 л/мин
    adjustedPower = min(adjustedPower, (uint8_t)50); // Максимум 50% мощности
  }
  
  // БЕЗОПАСНОСТЬ: При критически высокой температуре - принудительное снижение
  if (systemState.currentTemp > 75.0) {
    adjustedPower = min(adjustedPower, (uint8_t)MAX_POWER_AT_LOW_FLOW);
  }
  if (systemState.currentTemp > 80.0) {
    adjustedPower = min(adjustedPower, (uint8_t)MAX_POWER_AT_HIGH_TEMP);
  }
  
  // Целевая мощность для всех фаз
  uint8_t targetPower[3] = {adjustedPower, adjustedPower, adjustedPower};
  
  // Применение алгоритма Брезенхэма для плавного изменения мощности
  Utils::bresenhamPowerControl(systemState.heatingPower, targetPower, MAX_POWER_CHANGE_PER_CYCLE);
  
  lastPower = adjustedPower;
  
  if (DEBUG_SERIAL && millis() % 5000 < 100) {
    int powerDiff = adjustedPower - lastPower;
    Serial.printf("Адаптивный гистерезис: %.1f°C, Ошибка: %.1f°C, Мощность: %d%% (изменение: %d%%)\n", 
                  adaptiveHysteresis, tempError, adjustedPower, powerDiff);
  }
}

void emergencyShutdown() {
  if (systemState.isHeating) {
    if (DEBUG_SERIAL) {
      Serial.println("АВАРИЙНОЕ ОТКЛЮЧЕНИЕ!");
    }
    
    systemState.isHeating = false;
    systemState.isPowerRamping = false;
    
    // Аварийная остановка симисторов
    triacControl.emergencyStop();
    
    // Отключаем питание реле через MOSFET-модуль
    digitalWrite(MOSFET_EN_PIN, LOW);
  }
}

void loadConfiguration() {
  // Загрузка конфигурации из EEPROM
  if (systemState.loadConfiguration()) {
    if (DEBUG_SERIAL) {
      Serial.println("Конфигурация успешно загружена из EEPROM");
    }
  } else {
    if (DEBUG_SERIAL) {
      Serial.println("Используются настройки по умолчанию");
    }
  }
}

void saveConfiguration() {
  // Сохранение конфигурации в EEPROM
  if (systemState.saveConfiguration()) {
    if (DEBUG_SERIAL) {
      Serial.println("Конфигурация успешно сохранена в EEPROM");
    }
  } else {
    if (DEBUG_SERIAL) {
      Serial.println("Ошибка сохранения конфигурации в EEPROM");
    }
  }
}

// ===== ФУНКЦИИ ДЛЯ АРХИТЕКТУРЫ БЕЗ DEEP SLEEP =====

void handleSleepMode() {
  // Режим ожидания - постоянный мониторинг протока без deep sleep
  // Система остается активной и постоянно проверяет датчик протока
  static unsigned long lastFlowCheck = 0;
  static unsigned long lastDebugTime = 0;
  static unsigned long lastIdleCheck = 0;
  static unsigned long lastWatchdogReset = 0;
  
  if (millis() - lastFlowCheck >= 100) { // Проверяем проток каждые 100мс
    readFlowOnly();
    monitorZeroCross(); // Мониторинг детектора нуля
    lastFlowCheck = millis();
    
    // Отладочная информация каждые 5 секунд
    if (millis() - lastDebugTime >= 5000) {
      int flowSensorValue = digitalRead(FLOW_SENSOR_PIN);
      if (DEBUG_SERIAL) {
        Serial.printf("Режим ожидания: датчик протока=%d, проток=%.2f л/мин, обнаружен=%s\n", 
                      flowSensorValue, systemState.flowRate, 
                      systemState.isFlowDetected ? "ДА" : "НЕТ");
      }
      lastDebugTime = millis();
    }
    
    if (systemState.isFlowDetected) {
      // Обнаружен проток - переходим в активный режим
      systemState.systemMode = SYSTEM_MODE_ACTIVE;
      if (DEBUG_SERIAL) {
        Serial.println("Переход в активный режим - обнаружен проток");
      }
      return;
    }
  }
  
  // Проверка на переход в режим ожидания каждые 10 секунд
  if (millis() - lastIdleCheck >= 10000) {
    if (!systemState.isFlowDetected) {
      prepareForIdleMode();
    }
    lastIdleCheck = millis();
  }
  
  // Сброс watchdog каждые 2 секунды в режиме ожидания
  if (millis() - lastWatchdogReset >= 2000) {
    yield(); // Сброс watchdog
    lastWatchdogReset = millis();
  }
  
  // Небольшая задержка для экономии энергии без deep sleep
  delay(10);
}

void handleActiveMode(unsigned long currentTime) {
  // Активный режим - полная работа системы
  static bool activeModeLogged = false;
  static unsigned long lastWatchdogReset = 0;
  
  if (!activeModeLogged) {
    if (DEBUG_SERIAL) {
      Serial.println("АКТИВНЫЙ РЕЖИМ - обнаружен проток воды");
    }
    // Восстанавливаем полную частоту CPU для активной работы
    optimizeForActiveMode();
    activeModeLogged = true;
  }
  
  // Сброс watchdog каждые 500мс для предотвращения зависания
  if (currentTime - lastWatchdogReset >= 500) {
    yield(); // Сброс watchdog
    lastWatchdogReset = currentTime;
  }
  
  // Чтение всех датчиков
  if (currentTime - lastSensorRead >= 33) {
    readSensors();
    monitorZeroCross(); // Мониторинг детектора нуля
    lastSensorRead = currentTime;
  }
  
  // Обновление фазового управления симисторами (каждые 1мс для точности)
  static unsigned long lastTriacUpdate = 0;
  if (currentTime - lastTriacUpdate >= 1) {
    triacControl.updateTriacs();
    lastTriacUpdate = currentTime;
  }
  
  // Обработка флага прерывания детектора нуля
  triacControl.processZeroCrossFlag();
  
  // Мониторинг в Serial порт
  serialMonitor();
  
  // Проверка безопасности
  if (currentTime - systemState.lastSafetyCheck >= SAFETY_CHECK_INTERVAL) {
    safetyManager.checkSafety(systemState);
    systemState.lastSafetyCheck = currentTime;
  }
  
  // МГНОВЕННАЯ проверка протока в активном режиме (БЕЗОПАСНОСТЬ)
  // Используем аналоговую проверку протока для более точного определения
  if (systemState.isHeating && !systemState.isFlowDetected) {
    if (DEBUG_SERIAL) {
      Serial.printf("МГНОВЕННОЕ отключение в активном режиме - нет протока (%.2f л/мин)\n", systemState.flowRate);
    }
    stopHeating();
  }
  
  // Основная логика управления нагревом
  if (systemState.isSystemEnabled && systemState.isThermalFuseOK) {
    controlHeating();
  } else {
    emergencyShutdown();
  }
  
  // Проверяем, есть ли еще проток
  if (!systemState.isFlowDetected) {
    // Проток закончился - переходим в режим сна
    systemState.systemMode = SYSTEM_MODE_SLEEP;
    if (DEBUG_SERIAL) {
      Serial.println("Переход в режим сна - проток закончился");
    }
  }
  
  // Уменьшаем задержку для более быстрой реакции
  delay(5);
}

void handleWiFiSessionMode(unsigned long currentTime) {
  // Режим WiFi сессии - веб-сервер активен
  static bool wifiModeLogged = false;
  static unsigned long lastLedBlink = 0;
  static unsigned long lastConfigSave = 0;
  static unsigned long lastWatchdogReset = 0;
  
  if (!wifiModeLogged) {
    if (DEBUG_SERIAL) {
      Serial.println("WiFi СЕССИЯ - веб-интерфейс активен");
    }
    // Восстанавливаем полную частоту CPU для WiFi сессии
    optimizeForActiveMode();
    wifiModeLogged = true;
  }
  
  // Мигание светодиодом каждые 2 секунды для индикации активной WiFi сессии
  if (currentTime - lastLedBlink >= 2000) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastLedBlink = currentTime;
  }
  
  // Обработка веб-запросов
  webServer.handleClient();
  
  // Обновление статуса
  if (currentTime - lastWebUpdate >= 1000) {
    webServer.updateStatus(systemState);
    lastWebUpdate = currentTime;
  }
  
  // Автоматическое сохранение конфигурации каждые 30 секунд
  if (currentTime - lastConfigSave >= 30000) {
    systemState.saveConfiguration();
    lastConfigSave = currentTime;
  }
  
  // Также проверяем проток
  if (currentTime - lastSensorRead >= 100) {
    readSensors();
    monitorZeroCross(); // Мониторинг детектора нуля
    lastSensorRead = currentTime;
  }
  
  // Мониторинг в Serial порт
  serialMonitor();
  
  // Сброс watchdog каждую секунду в WiFi режиме
  if (currentTime - lastWatchdogReset >= 1000) {
    yield(); // Сброс watchdog
    lastWatchdogReset = currentTime;
  }
  
  // В WiFi режиме НЕ переключаемся автоматически в активный режим
  // Пользователь может управлять системой через веб-интерфейс
  // Автоматический переход отключен для стабильной работы WiFi сессии
}

void checkBootButton() {
  static bool lastButtonState = HIGH;
  static unsigned long lastPressTime = 0;
  static unsigned long lastReleaseTime = 0;
  
  bool currentButtonState = digitalRead(BOOT_BUTTON_PIN);
  
  // Обнаружение нажатия (переход с HIGH на LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    unsigned long currentTime = millis();
    
    if (DEBUG_SERIAL) {
      Serial.printf("Кнопка BOOT нажата в %lu\n", currentTime);
    }
    
    lastPressTime = currentTime;
  }
  
  // Обнаружение отпускания (переход с LOW на HIGH)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    unsigned long currentTime = millis();
    unsigned long pressDuration = currentTime - lastPressTime;
    
    if (DEBUG_SERIAL) {
      Serial.printf("Кнопка BOOT отпущена, длительность: %lu мс\n", pressDuration);
    }
    
    // Проверяем, было ли это двойное нажатие
    if (currentTime - lastReleaseTime < BOOT_DOUBLE_CLICK_TIMEOUT_MS) {
      if (DEBUG_SERIAL) {
        Serial.println("Двойное нажатие BOOT - запуск WiFi сессии!");
      }
      enableWiFiSession();
    }
    
    lastReleaseTime = currentTime;
  }
  
  lastButtonState = currentButtonState;
}

void handleBootButtonWakeup() {
  // Обработка пробуждения по кнопке BOOT
  if (DEBUG_SERIAL) {
    Serial.println("Пробуждение по кнопке BOOT");
  }
  
  systemState.buttonPressCount = 1;
  systemState.lastButtonPress = millis();
  
  // Ждем возможного второго нажатия
  delay(1000);
  
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    // Кнопка все еще нажата - это двойное нажатие
    if (DEBUG_SERIAL) {
      Serial.println("Двойное нажатие BOOT - запуск WiFi сессии");
    }
    systemState.buttonPressCount = 2;
    enableWiFiSession();
  } else {
    // Одиночное нажатие - обычный режим
    if (DEBUG_SERIAL) {
      Serial.println("Одиночное нажатие BOOT - обычный режим");
    }
    systemState.systemMode = SYSTEM_MODE_SLEEP;
  }
}

void enableWiFiSession() {
  if (DEBUG_SERIAL) {
    Serial.println("Запуск WiFi сессии на 15 минут");
  }
  
  systemState.isWiFiEnabled = true;
  systemState.wifiSessionStartTime = millis();
  systemState.systemMode = SYSTEM_MODE_WIFI_SESSION;
  
  if (DEBUG_SERIAL) {
    Serial.printf("WiFi сессия запущена в %lu мс\n", systemState.wifiSessionStartTime);
  }
  
  // Включаем светодиод для индикации активной WiFi сессии
  digitalWrite(STATUS_LED_PIN, HIGH);
  
  // Запуск WiFi с полной мощностью
  webServer.begin();
}

void disableWiFiSession() {
  if (DEBUG_SERIAL) {
    Serial.println("Завершение WiFi сессии");
    Serial.printf("Сессия длилась: %lu мс\n", millis() - systemState.wifiSessionStartTime);
  }
  
  systemState.isWiFiEnabled = false;
  systemState.systemMode = SYSTEM_MODE_SLEEP;
  
  // Выключаем светодиод
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Остановка веб-сервера
  WiFi.mode(WIFI_OFF);
}

void prepareForIdleMode() {
  // Проверяем, не была ли уже подготовлена система к режиму ожидания
  if (cpuOptimizedForIdle) {
    return; // Уже подготовлена, не делаем повторно
  }
  
  if (DEBUG_SERIAL) {
    Serial.println("Переход в режим ожидания...");
  }
  
  // Проверяем состояние датчика протока
  int flowSensorState = digitalRead(FLOW_SENSOR_PIN);
  if (DEBUG_SERIAL) {
    Serial.printf("Состояние датчика протока: %d\n", flowSensorState);
    Serial.println("Система остается активной для постоянного мониторинга");
  }
  
  // Отключаем deep sleep - система остается активной
  systemState.isInDeepSleep = false;
  
  // Оптимизация энергопотребления - снижаем частоту CPU
  setCpuFrequencyMhz(80); // Снижаем с 240MHz до 80MHz для экономии энергии
  
  if (DEBUG_SERIAL) {
    Serial.println("Режим ожидания активирован - постоянный мониторинг датчиков");
    Serial.printf("CPU частота снижена до: %d MHz\n", getCpuFrequencyMhz());
  }
  
  // Отмечаем, что режим ожидания уже подготовлен
  cpuOptimizedForIdle = true;
}

void optimizeForActiveMode() {
  // Восстанавливаем полную частоту CPU для активной работы
  setCpuFrequencyMhz(240);
  
  // Сбрасываем флаг подготовки режима ожидания
  cpuOptimizedForIdle = false;
  
  if (DEBUG_SERIAL) {
    Serial.printf("CPU частота восстановлена до: %d MHz\n", getCpuFrequencyMhz());
  }
}
