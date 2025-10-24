#include "triac_control.h"
#include "system_state.h"
#include "utils.h"
#include <Arduino.h>
#include <HardwareSerial.h>

// Статическая переменная для доступа к объекту из прерывания
TriacControl* TriacControl::instance = nullptr;

void TriacControl::begin() {
  // Инициализация статической переменной для прерывания
  instance = this;
  
  // Настройка пинов симисторов как цифровых выходов
  pinMode(TRIAC_L1_PIN, OUTPUT);
  pinMode(TRIAC_L2_PIN, OUTPUT);
  pinMode(TRIAC_L3_PIN, OUTPUT);
  
  // Изначально все выключено
  digitalWrite(TRIAC_L1_PIN, LOW);
  digitalWrite(TRIAC_L2_PIN, LOW);
  digitalWrite(TRIAC_L3_PIN, LOW);
  
  // Включаем прерывание детектора нуля
  enableZeroCrossInterrupt();
  
  if (DEBUG_SERIAL) {
    Serial.println("Управление симисторами инициализировано");
  }
}

void TriacControl::setPower(uint8_t power[3]) {
  // Ограничение мощности
  uint8_t limitedPower[3];
  for (int i = 0; i < 3; i++) {
    limitedPower[i] = constrain(power[i], 0, MAX_POWER_PERCENT);
  }
  
  // Плавное изменение мощности (ramp)
  rampPower(limitedPower);
}

void TriacControl::setPhasePower(uint8_t phase, uint8_t power) {
  if (phase >= 0 && phase < 3) {
    currentPower[phase] = constrain(power, 0, MAX_POWER_PERCENT);
    updateTriacs();
  }
}

void TriacControl::normalStop() {
  if (DEBUG_SERIAL) {
    Serial.println("Плановая остановка симисторов");
  }
  
  // Плавное отключение всех симисторов
  digitalWrite(TRIAC_L1_PIN, LOW);
  digitalWrite(TRIAC_L2_PIN, LOW);
  digitalWrite(TRIAC_L3_PIN, LOW);
  
  // Сброс текущей мощности
  for (int i = 0; i < 3; i++) {
    currentPower[i] = 0;
  }
}

void TriacControl::emergencyStop() {
  if (DEBUG_SERIAL) {
    Serial.println("АВАРИЙНАЯ ОСТАНОВКА СИМИСТОРОВ");
  }
  
  // Немедленное отключение всех симисторов
  digitalWrite(TRIAC_L1_PIN, LOW);
  digitalWrite(TRIAC_L2_PIN, LOW);
  digitalWrite(TRIAC_L3_PIN, LOW);
  
  // Сброс текущей мощности
  for (int i = 0; i < 3; i++) {
    currentPower[i] = 0;
  }
}

bool TriacControl::isPhaseActive(uint8_t phase) {
  if (phase >= 0 && phase < 3) {
    return currentPower[phase] > 0;
  }
  return false;
}

void TriacControl::updateTriacs() {
  // Фазовое управление симисторами BTA41-800B через MOC3023
  // Используем детектор нуля H11AA1 для синхронизации с сетью 220В
  
  static bool triacFired[3] = {false, false, false};
  static unsigned long lastZeroCrossTime = 0;
  unsigned long currentMicros = micros();
  
  // Проверяем, прошло ли достаточно времени с последнего пересечения нуля
  if (lastZeroCrossMicros > 0) {
    // Безопасный расчет времени с учетом переполнения micros()
    unsigned long timeSinceZeroCross;
    if (currentMicros >= lastZeroCrossMicros) {
      timeSinceZeroCross = currentMicros - lastZeroCrossMicros;
    } else {
      // Переполнение micros() - считаем от 0
      timeSinceZeroCross = currentMicros + (0xFFFFFFFF - lastZeroCrossMicros);
    }
    
    // Сбрасываем флаги срабатывания при новом пересечении нуля
    if (lastZeroCrossMicros != lastZeroCrossTime) {
      for (int i = 0; i < 3; i++) {
        triacFired[i] = false;
      }
      lastZeroCrossTime = lastZeroCrossMicros;
    }
    
    // Если прошло больше 10мс (полупериод), сбрасываем флаги
    if (timeSinceZeroCross > 10000) {
      for (int i = 0; i < 3; i++) {
        triacFired[i] = false;
      }
    }
    
    for (int i = 0; i < 3; i++) {
      if (currentPower[i] == 0) {
        // Выключено - отключаем симистор
        digitalWrite(TRIAC_L1_PIN + i, LOW);
        triacFired[i] = false;
      } else if (currentPower[i] == 100) {
        // Полностью включено - включаем сразу после пересечения нуля
        if (timeSinceZeroCross < 1000) { // В течение 1мс после пересечения нуля
          digitalWrite(TRIAC_L1_PIN + i, HIGH);
          triacFired[i] = true;
        } else {
          digitalWrite(TRIAC_L1_PIN + i, LOW);
        }
      } else {
        // Фазовое управление для промежуточных значений
        if (!triacFired[i]) {
          // Вычисляем задержку включения в микросекундах
          // 0% = 10000мкс (не включаем), 100% = 0мкс (включаем сразу)
          unsigned long delayMicros = map(currentPower[i], 0, 100, 10000, 0);
          
          // Включаем симистор если прошло достаточно времени
          if (timeSinceZeroCross >= delayMicros && timeSinceZeroCross < 8000) {
            digitalWrite(TRIAC_L1_PIN + i, HIGH);
            triacFired[i] = true; // Отмечаем, что симистор сработал в этом полупериоде
          } else {
            digitalWrite(TRIAC_L1_PIN + i, LOW);
          }
        } else {
          // Симистор уже сработал в этом полупериоде - выключаем
          digitalWrite(TRIAC_L1_PIN + i, LOW);
        }
      }
    }
  }
  
  lastUpdateTime = millis();
  
  if (DEBUG_SERIAL && millis() % 2000 < 100) {
    unsigned long timeSinceZeroCross = 0;
    if (lastZeroCrossMicros > 0) {
      if (currentMicros >= lastZeroCrossMicros) {
        timeSinceZeroCross = currentMicros - lastZeroCrossMicros;
      } else {
        timeSinceZeroCross = currentMicros + (0xFFFFFFFF - lastZeroCrossMicros);
      }
    }
    Serial.printf("СИМИСТОРЫ: L1=%d%%, L2=%d%%, L3=%d%% (Время: %lu мкс, Ноль: %lu)\n", 
                  currentPower[0], currentPower[1], currentPower[2], 
                  timeSinceZeroCross, lastZeroCrossMicros);
  }
}

void TriacControl::rampPower(uint8_t targetPower[3]) {
  // Плавное изменение мощности с использованием алгоритма Брезенхэма
  Utils::bresenhamPowerControl(currentPower, targetPower, MAX_POWER_CHANGE_PER_CYCLE);
  updateTriacs();
}

void TriacControl::smoothRampToPower(uint8_t targetPower[3], SystemState& state) {
  // Проверяем, нужно ли изменить целевую мощность
  bool needsRamp = false;
  for (int i = 0; i < 3; i++) {
    if (state.targetPower[i] != targetPower[i]) {
      needsRamp = true;
      break;
    }
  }
  
  // Запускаем разгон только если нужно изменить целевую мощность
  if (needsRamp) {
    // Установка целевой мощности и запуск плавного изменения
    for (int i = 0; i < 3; i++) {
      state.targetPower[i] = targetPower[i];
    }
    state.isPowerRamping = true;
    state.lastPowerRampTime = millis();
    
    if (DEBUG_SERIAL) {
      Serial.printf("Запуск плавного разгона до мощности: L1=%d%%, L2=%d%%, L3=%d%% (2 сек)\n", 
                    targetPower[0], targetPower[1], targetPower[2]);
    }
  }
}

void TriacControl::updateRamp(SystemState& state) {
  if (!state.isPowerRamping) return;
  
  unsigned long currentTime = millis();
  
  // БЕЗОПАСНОСТЬ: Проверяем таймаут разгона
  if (currentTime - state.heatingStartTime > POWER_RAMP_TIMEOUT_MS) {
    if (DEBUG_SERIAL) {
      Serial.println("БЕЗОПАСНОСТЬ: Таймаут разгона - принудительное завершение");
    }
    state.isPowerRamping = false;
    return;
  }
  
  // Проверяем, нужно ли обновить мощность
  if (currentTime - state.lastPowerRampTime >= POWER_RAMP_STEP_MS) {
    bool rampComplete = true;
    
    // Проверяем, достигли ли мы целевой мощности
    for (int i = 0; i < 3; i++) {
      if (currentPower[i] != state.targetPower[i]) {
        rampComplete = false;
        break;
      }
    }
    
    if (!rampComplete) {
      // Используем алгоритм Брезенхэма для плавного изменения мощности
      uint8_t step = 1; // Базовый шаг
      
      // При малом протоке - более консервативный шаг
      if (state.flowRate < FLOW_THRESHOLD_MIN) {
        step = 1; // Максимум 1% при малом протоке
      } else {
        step = 3; // Максимум 3% при нормальном протоке
      }
      
      Utils::bresenhamPowerControl(currentPower, state.targetPower, step);
      updateTriacs();
      
      // Обновляем мощность в состоянии системы
      for (int i = 0; i < 3; i++) {
        state.heatingPower[i] = currentPower[i];
      }
    }
    
    if (rampComplete) {
      state.isPowerRamping = false;
      // Обновляем мощность в состоянии системы
      for (int i = 0; i < 3; i++) {
        state.heatingPower[i] = currentPower[i];
      }
      if (DEBUG_SERIAL) {
        Serial.println("Плавный разгон завершен - достигнута целевая мощность");
      }
    }
    
    state.lastPowerRampTime = currentTime;
  }
}

// Простые функции для работы с детектором нуля H11AA1
bool TriacControl::readZeroCrossState() {
  // Читаем цифровое значение для H11AA1
  int digitalValue = digitalRead(ZERO_CROSS_PIN);
  
  // Логика для H11AA1 (оптоизолятор с встроенным pull-up):
  // При наличии 220В -> H11AA1 активен -> выход LOW (0) -> возвращаем false
  // При отсутствии 220В -> H11AA1 неактивен -> выход HIGH (1) -> возвращаем true
  return digitalValue == HIGH;
}

int TriacControl::getZeroCrossRawValue() {
  // Возвращаем сырое значение для диагностики
  return digitalRead(ZERO_CROSS_PIN);
}

void TriacControl::enableZeroCrossInterrupt() {
  if (!zeroCrossEnabled) {
    // Настройка прерывания для детектора нуля (H11AA1)
    // FALLING - срабатывает при переходе с HIGH на LOW (пересечение нуля)
    attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_PIN), zeroCrossInterrupt, FALLING);
    zeroCrossEnabled = true;
    
    if (DEBUG_SERIAL) {
      Serial.println("Прерывание детектора нуля включено (FALLING)");
    }
  }
}

void TriacControl::disableZeroCrossInterrupt() {
  if (zeroCrossEnabled) {
    detachInterrupt(digitalPinToInterrupt(ZERO_CROSS_PIN));
    zeroCrossEnabled = false;
    
    if (DEBUG_SERIAL) {
      Serial.println("Прерывание детектора нуля отключено");
    }
  }
}

void TriacControl::zeroCrossInterrupt() {
  if (instance != nullptr) {
    // Записываем время пересечения нуля в микросекундах
    instance->lastZeroCrossMicros = micros();
      // Устанавливаем флаг для логирования в основном цикле
    instance->zeroCrossFlag = true;
  }
}

void TriacControl::processZeroCrossFlag() {
  if (zeroCrossFlag) {
    zeroCrossFlag = false;
    
    // Диагностика прерывания в основном цикле
    static unsigned long lastInterruptTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTime > 1000) { // Логируем не чаще раза в секунду
      if (DEBUG_SERIAL) {
        // Показываем относительное время с момента последнего пересечения нуля
        unsigned long timeSinceLastZero = micros() - lastZeroCrossMicros;
        Serial.printf("ПРЕРЫВАНИЕ ДЕТЕКТОРА НУЛЯ: Время=%lu мкс (относительно: %lu мкс)\n", 
                      lastZeroCrossMicros, timeSinceLastZero);
      }
      lastInterruptTime = currentTime;
    }
  }
}

void TriacControl::testTriacs() {
  if (DEBUG_SERIAL) {
    Serial.println("=== ТЕСТ СИМИСТОРОВ ===");
    Serial.printf("Пины: L1=%d, L2=%d, L3=%d\n", TRIAC_L1_PIN, TRIAC_L2_PIN, TRIAC_L3_PIN);
    Serial.printf("Детектор нуля: пин=%d, значение=%d, состояние=%s\n", 
                  ZERO_CROSS_PIN, getZeroCrossRawValue(), 
                  readZeroCrossState() ? "ВКЛ 220В" : "ВЫКЛ 220В");
    Serial.printf("Время последнего пересечения нуля: %lu мкс\n", lastZeroCrossMicros);
    
    // Тест включения симисторов
    Serial.println("Тест включения симисторов на 1 секунду...");
    digitalWrite(TRIAC_L1_PIN, HIGH);
    digitalWrite(TRIAC_L2_PIN, HIGH);
    digitalWrite(TRIAC_L3_PIN, HIGH);
    delay(1000);
    
    Serial.println("Отключение симисторов...");
    digitalWrite(TRIAC_L1_PIN, LOW);
    digitalWrite(TRIAC_L2_PIN, LOW);
    digitalWrite(TRIAC_L3_PIN, LOW);
    Serial.println("=== КОНЕЦ ТЕСТА ===");
  }
}
