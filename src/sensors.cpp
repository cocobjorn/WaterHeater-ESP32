#include "sensors.h"
#include <Arduino.h>
#include <HardwareSerial.h>

// Статическая переменная для доступа к объекту из прерывания
Sensors* Sensors::instance = nullptr;

void Sensors::begin() {
  instance = this;
  
  // Настройка прерывания для датчика протока
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowInterrupt, FALLING);
  
  // Инициализация завершена
  
  // Инициализация фильтров
  for (int i = 0; i < 5; i++) {
    tempFilter[i] = 25.0; // Начальная температура
  }
  for (int i = 0; i < 3; i++) {
    flowFilter[i] = 0.0;
  }
  
  if (DEBUG_SERIAL) {
    Serial.println("Датчики инициализированы");
  }
}

float Sensors::readTemperature() {
  // Чтение ADC значения
  int adcValue = analogRead(NTC_PIN);
  
  // Преобразование в напряжение (ESP32: 0-4095 -> 0-3.3V)
  float voltage = (adcValue / 4095.0) * 3.3;
  
  // Расчет сопротивления NTC
  float resistance = (NTC_SERIES_RESISTANCE * voltage) / (3.3 - voltage);
  
  // Расчет температуры по формуле Стейнхарта-Харта
  float rawTemperature = calculateNTCtemperature(resistance);
  
  // Применение медианного фильтра для стабильности
  tempFilter[tempFilterIndex] = rawTemperature;
  tempFilterIndex = (tempFilterIndex + 1) % 5;
  
  // Сортировка и выбор медианы
  float sorted[5];
  for (int i = 0; i < 5; i++) {
    sorted[i] = tempFilter[i];
  }
  
  // Простая сортировка пузырьком
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4 - i; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  float filteredTemperature = sorted[2]; // Медиана
  
  // Возвращаем отфильтрованную температуру без компенсации
  return filteredTemperature;
}

float Sensors::readFlowRate() {
  unsigned long currentTime = millis();
  
  // Если прошло больше 1 секунды с последнего измерения
  if (currentTime - lastFlowTime >= 1000) {
    // Расчет протока: импульсы в секунду / калибровочный коэффициент
    float flowRate = (float)flowPulseCount / flowCalibrationFactor;
    
    // Применение фильтра скользящего среднего
    flowFilter[flowFilterIndex] = flowRate;
    flowFilterIndex = (flowFilterIndex + 1) % 3;
    
    float filteredFlow = 0;
    for (int i = 0; i < 3; i++) {
      filteredFlow += flowFilter[i];
    }
    filteredFlow /= 3.0;
    
    // Сброс счетчика
    flowPulseCount = 0;
    lastFlowTime = currentTime;
    
    return filteredFlow;
  }
  
  // Возврат последнего значения
  return flowFilter[(flowFilterIndex + 2) % 3];
}

void Sensors::calibrateFlowSensor() {
  if (DEBUG_SERIAL) {
    Serial.println("Калибровка датчика протока...");
    Serial.println("Пропустите 1 литр воды через датчик");
  }
  
  unsigned long startTime = millis();
  unsigned long startPulses = flowPulseCount;
  
  // Ждем 30 секунд или пока не наберется достаточно импульсов
  while ((millis() - startTime) < 30000 && (flowPulseCount - startPulses) < 50) {
    delay(100);
  }
  
  if (flowPulseCount > startPulses) {
    flowCalibrationFactor = (float)(flowPulseCount - startPulses);
    if (DEBUG_SERIAL) {
      Serial.print("Калибровка завершена. Коэффициент: ");
      Serial.println(flowCalibrationFactor);
    }
  } else {
    if (DEBUG_SERIAL) {
      Serial.println("Ошибка калибровки - недостаточно данных");
    }
  }
}

void Sensors::setCalibrationFactor(float factor) {
  flowCalibrationFactor = constrain(factor, FLOW_CALIBRATION_MIN, FLOW_CALIBRATION_MAX);
  if (DEBUG_SERIAL) {
    Serial.print("Коэффициент калибровки установлен: ");
    Serial.println(flowCalibrationFactor);
  }
}

float Sensors::getCalibrationFactor() {
  return flowCalibrationFactor;
}

unsigned long Sensors::getFlowPulseCount() {
  return flowPulseCount;
}

void Sensors::resetFlowPulseCount() {
  flowPulseCount = 0;
  if (DEBUG_SERIAL) {
    Serial.println("Счетчик импульсов протока сброшен");
  }
}

float Sensors::calculateNTCtemperature(float resistance) {
  // Формула Стейнхарта-Харта для NTC термистора
  float steinhart = log(resistance / NTC_NOMINAL_RESISTANCE) / NTC_BETA;
  steinhart += 1.0 / (NTC_NOMINAL_TEMP + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;
  
  return steinhart;
}

void Sensors::flowInterrupt() {
  if (instance != nullptr) {
    instance->flowPulseCount++;
  }
}
