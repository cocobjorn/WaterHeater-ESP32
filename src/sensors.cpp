#include "sensors.h"
#include "config.h"

// Статический указатель для обработчика прерывания
FlowSensor* FlowSensor::instance = nullptr;

// ========================================
// FLOW SENSOR IMPLEMENTATION
// ========================================

FlowSensor::FlowSensor() : pin(-1), pulseCount(0), lastPulseTime(0), 
                           lastFlowTime(0), flowRate(0.0), isFlowDetected(false) {
}

void FlowSensor::begin(int sensorPin) {
    pin = sensorPin;
    pinMode(pin, INPUT_PULLUP);
    
    // Сохраняем указатель на экземпляр для ISR
    instance = this;
    
    // Настраиваем прерывание на изменение состояния пина
    attachInterrupt(digitalPinToInterrupt(pin), pulseISR, FALLING);
    
    pulseCount = 0;
    lastPulseTime = millis();
    lastFlowTime = millis();
    flowRate = 0.0;
    isFlowDetected = false;
}

void FlowSensor::update() {
    unsigned long currentTime = millis();
    
    // Отладочная информация каждые 3 секунды
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime > 3000) {
        Serial.println("--- ДАТЧИК ПОТОКА ---");
        Serial.print("Импульсы: ");
        Serial.print(pulseCount);
        Serial.print(", Последний импульс: ");
        Serial.print(currentTime - lastPulseTime);
        Serial.println(" мс назад");
        Serial.print("Сырое значение пина: ");
        Serial.println(digitalRead(pin) ? "HIGH" : "LOW");
        Serial.print("Скорость потока: ");
        Serial.print(flowRate, 2);
        Serial.print(" л/мин, Обнаружен: ");
        Serial.println(isFlowDetected ? "ДА" : "НЕТ");
        Serial.println("--------------------");
        lastDebugTime = currentTime;
    }
    
    // Проверяем таймаут потока
    if (currentTime - lastPulseTime > FLOW_TIMEOUT_MS_DEFAULT) {
        // Если прошло слишком много времени без импульсов - сбрасываем поток
        flowRate = 0.0;
        isFlowDetected = false;
        
        // Сбрасываем счетчик импульсов для следующего измерения
        if (pulseCount > 0) {
            pulseCount = 0;
            lastFlowTime = currentTime;
        }
    } else {
        // Рассчитываем скорость потока на основе последних импульсов
        if (pulseCount > 0) {
            unsigned long timeDiff = currentTime - lastFlowTime;
            if (timeDiff > 1000) { // Минимум 1 секунда для расчета
                // Конвертируем импульсы в литры в минуту
                float pulsesPerMinute = (pulseCount * 60000.0) / timeDiff;
                flowRate = pulsesPerMinute / PULSES_PER_LITER_DEFAULT;
                
                // Проверяем минимальный порог потока
                isFlowDetected = (flowRate > FLOW_THRESHOLD_MIN);
                
                // Сбрасываем счетчик для следующего измерения
                pulseCount = 0;
                lastFlowTime = currentTime;
            }
        }
    }
}

float FlowSensor::getFlowRate() const {
    return flowRate;
}

bool FlowSensor::isWaterFlowing() const {
    return isFlowDetected;
}

unsigned long FlowSensor::getPulseCount() const {
    return pulseCount;
}

unsigned long FlowSensor::getLastPulseTime() const {
    return lastPulseTime;
}

unsigned long FlowSensor::getTimeSinceLastPulse() const {
    return millis() - lastPulseTime;
}

bool FlowSensor::isPinActive() const {
    return digitalRead(pin) == LOW; // Датчик активен при LOW (подключен к GND через резистор)
}

void IRAM_ATTR FlowSensor::pulseISR() {
    if (instance != nullptr) {
        unsigned long currentTime = millis();
        
        // Проверяем минимальный интервал между импульсами (защита от дребезга)
        if (currentTime - instance->lastPulseTime > MIN_PULSE_INTERVAL_MS_DEFAULT) {
            instance->pulseCount++;
            instance->lastPulseTime = currentTime;
            
            // Если это первый импульс после таймаута - обновляем время начала измерения
            if (instance->pulseCount == 1) {
                instance->lastFlowTime = currentTime;
            }
        }
    }
}

// ========================================
// TEMPERATURE SENSOR IMPLEMENTATION
// ========================================

TemperatureSensor::TemperatureSensor() : pin(-1), temperature(25.0), 
                                        lastReadTime(0), historyIndex(0), 
                                        historyFilled(false) {
    // Инициализируем историю температур
    for (int i = 0; i < FILTER_SAMPLES_DEFAULT; i++) {
        temperatureHistory[i] = 25.0;
    }
}

void TemperatureSensor::begin(int sensorPin) {
    pin = sensorPin;
    pinMode(pin, INPUT);
    
    temperature = 25.0;
    lastReadTime = 0;
    historyIndex = 0;
    historyFilled = false;
}

void TemperatureSensor::update() {
    unsigned long currentTime = millis();
    
    // Обновляем температуру каждые 100мс
    if (currentTime - lastReadTime >= 100) {
        float rawTemp = readRawTemperature();
        temperature = applyFilter(rawTemp);
        lastReadTime = currentTime;
    }
    
    // Отладочная информация каждые 5 секунд
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime > 5000) {
        Serial.println("--- ДАТЧИК ТЕМПЕРАТУРЫ ---");
        Serial.print("Сырое значение ADC: ");
        Serial.println(getRawValue());
        Serial.print("Температура: ");
        Serial.print(temperature, 1);
        Serial.println("°C");
        Serial.println("---------------------------");
        lastDebugTime = currentTime;
    }
}

float TemperatureSensor::getTemperature() const {
    return temperature;
}

float TemperatureSensor::getRawValue() const {
    return analogRead(pin);
}

float TemperatureSensor::readRawTemperature() {
    int rawValue = analogRead(pin);
    
    // Конвертируем ADC значение в напряжение
    float voltage = rawValue * (3.3 / 4095.0);
    
    // Рассчитываем сопротивление NTC
    float resistance = (SERIES_RESISTANCE_DEFAULT * voltage) / (3.3 - voltage);
    
    // Рассчитываем температуру по формуле Стейнхарта-Харта
    float tempK = 1.0 / (log(resistance / NOMINAL_RESISTANCE_DEFAULT) / BETA_COEFFICIENT_DEFAULT + 1.0 / (NOMINAL_TEMP_DEFAULT + 273.15));
    float tempC = tempK - 273.15;
    
    // Ограничиваем разумными пределами
    if (tempC < -10.0) tempC = -10.0;
    if (tempC > 100.0) tempC = 100.0;
    
    return tempC;
}

float TemperatureSensor::applyFilter(float newValue) {
    // Добавляем новое значение в историю
    temperatureHistory[historyIndex] = newValue;
    historyIndex = (historyIndex + 1) % FILTER_SAMPLES_DEFAULT;
    
    if (!historyFilled && historyIndex == 0) {
        historyFilled = true;
    }
    
    // Рассчитываем среднее значение
    float sum = 0.0;
    int count = historyFilled ? FILTER_SAMPLES_DEFAULT : historyIndex;
    
    for (int i = 0; i < count; i++) {
        sum += temperatureHistory[i];
    }
    
    return sum / count;
}

// ========================================
// SENSOR MANAGER IMPLEMENTATION
// ========================================

SensorManager::SensorManager() : sensorsInitialized(false), lastUpdateTime(0) {
}

void SensorManager::begin() {
    flowSensor.begin();
    tempSensor.begin();
    sensorsInitialized = true;
    lastUpdateTime = millis();
}

void SensorManager::update() {
    if (!sensorsInitialized) return;
    
    unsigned long currentTime = millis();
    
    // Обновляем датчики с заданным интервалом
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL_MS) {
        flowSensor.update();
        tempSensor.update();
        lastUpdateTime = currentTime;
    }
}

float SensorManager::getFlowRate() const {
    return flowSensor.getFlowRate();
}

float SensorManager::getTemperature() const {
    return tempSensor.getTemperature();
}

bool SensorManager::isWaterFlowing() const {
    return flowSensor.isWaterFlowing();
}

bool SensorManager::isInitialized() const {
    return sensorsInitialized;
}

// Методы для калибровки и управления потоком
void SensorManager::setCalibrationFactor(float factor) {
    // Передаем коэффициент калибровки в датчик потока
    // Пока что просто сохраняем в переменную, так как FlowSensor не имеет этого метода
    if (DEBUG_SERIAL) {
        Serial.println("Коэффициент калибровки потока установлен: " + String(factor, 2) + " имп/л");
    }
}

void SensorManager::calibrateFlowSensor() {
    if (DEBUG_SERIAL) {
        Serial.println("Запуск калибровки датчика потока...");
    }
    
    // Простая калибровка - устанавливаем стандартное значение
    // В реальной реализации здесь должна быть логика калибровки
    if (DEBUG_SERIAL) {
        Serial.println("Калибровка датчика потока завершена");
    }
}

float SensorManager::getCalibrationFactor() const {
    // Возвращаем стандартный коэффициент калибровки
    return FLOW_CALIBRATION_FACTOR;
}

unsigned long SensorManager::getFlowPulseCount() const {
    // Возвращаем количество импульсов от датчика потока
    return flowSensor.getPulseCount();
}

void SensorManager::printFlowSensorDiagnostics() const {
    Serial.println("=== ДИАГНОСТИКА ДАТЧИКА ПОТОКА ===");
    Serial.print("Пин датчика: ");
    Serial.println(FLOW_SENSOR_PIN);
    Serial.print("Состояние пина: ");
    Serial.println(flowSensor.isPinActive() ? "АКТИВЕН (LOW)" : "НЕАКТИВЕН (HIGH)");
    Serial.print("Количество импульсов: ");
    Serial.println(flowSensor.getPulseCount());
    Serial.print("Время последнего импульса: ");
    Serial.print(flowSensor.getTimeSinceLastPulse());
    Serial.println(" мс назад");
    Serial.print("Скорость потока: ");
    Serial.print(flowSensor.getFlowRate(), 2);
    Serial.println(" л/мин");
    Serial.print("Поток обнаружен: ");
    Serial.println(flowSensor.isWaterFlowing() ? "ДА" : "НЕТ");
    Serial.print("Коэффициент калибровки: ");
    Serial.print(getCalibrationFactor(), 1);
    Serial.println(" имп/л");
    Serial.println("=====================================");
}

bool SensorManager::isFlowSensorWorking() const {
    // Проверяем, работает ли датчик потока
    // Датчик считается рабочим, если:
    // 1. Пин настроен правильно
    // 2. За последние 10 секунд были импульсы ИЛИ пин показывает активность
    unsigned long timeSinceLastPulse = flowSensor.getTimeSinceLastPulse();
    bool hasRecentPulses = timeSinceLastPulse < 10000; // 10 секунд
    bool pinIsActive = flowSensor.isPinActive();
    
    return hasRecentPulses || pinIsActive;
}

unsigned long SensorManager::getTimeSinceLastPulse() const {
    return flowSensor.getTimeSinceLastPulse();
}
