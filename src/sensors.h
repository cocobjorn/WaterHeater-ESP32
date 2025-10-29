#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "config.h"

class FlowSensor {
private:
    int pin;
    volatile unsigned long pulseCount;
    volatile unsigned long lastPulseTime;
    unsigned long lastFlowTime;
    float flowRate; // л/мин
    bool isFlowDetected;
    
    // Настройки датчика
    static constexpr float PULSES_PER_LITER_DEFAULT = PULSES_PER_LITER; // Импульсов на литр
    static constexpr unsigned long FLOW_TIMEOUT_MS_DEFAULT = FLOW_TIMEOUT_MS; // Таймаут отсутствия потока
    static constexpr unsigned long MIN_PULSE_INTERVAL_MS_DEFAULT = MIN_PULSE_INTERVAL_MS; // Минимальный интервал между импульсами
    
public:
    FlowSensor();
    
    void begin(int sensorPin = FLOW_SENSOR_PIN);
    void update();
    
    // Получение данных
    float getFlowRate() const; // л/мин
    bool isWaterFlowing() const;
    unsigned long getPulseCount() const;
    
    // Диагностические методы
    unsigned long getLastPulseTime() const;
    unsigned long getTimeSinceLastPulse() const;
    bool isPinActive() const;
    
    // Обработчик прерывания
    static void IRAM_ATTR pulseISR();
    static FlowSensor* instance;
};

class TemperatureSensor {
private:
    int pin;
    float temperature;
    unsigned long lastReadTime;
    
    // Параметры NTC датчика
    static constexpr float NOMINAL_TEMP_DEFAULT = NOMINAL_TEMP; // Номинальная температура
    static constexpr float NOMINAL_RESISTANCE_DEFAULT = NOMINAL_RESISTANCE; // Сопротивление при 25°C
    static constexpr float BETA_COEFFICIENT_DEFAULT = BETA_COEFFICIENT; // Бета-коэффициент
    static constexpr float SERIES_RESISTANCE_DEFAULT = SERIES_RESISTANCE; // Подтягивающий резистор
    
    // Фильтрация
    static const int FILTER_SAMPLES_DEFAULT = FILTER_SAMPLES;
    float temperatureHistory[FILTER_SAMPLES];
    int historyIndex;
    bool historyFilled;
    
public:
    TemperatureSensor();
    
    void begin(int sensorPin = NTC_PIN);
    void update();
    
    // Получение данных
    float getTemperature() const; // °C
    float getRawValue() const;
    
private:
    float readRawTemperature();
    float applyFilter(float newValue);
};

class SensorManager {
private:
    FlowSensor flowSensor;
    TemperatureSensor tempSensor;
    
    // Состояние системы
    bool sensorsInitialized;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL_MS = 100; // Обновление каждые 100мс
    
public:
    SensorManager();
    
    void begin();
    void update();
    
    // Получение данных
    float getFlowRate() const;
    float getTemperature() const;
    bool isWaterFlowing() const;
    
    // Методы для калибровки и управления потоком
    void setCalibrationFactor(float factor);
    void calibrateFlowSensor();
    float getCalibrationFactor() const;
    unsigned long getFlowPulseCount() const;
    
    // Проверка состояния
    bool isInitialized() const;
    
    // Расширенная диагностика датчика потока
    void printFlowSensorDiagnostics() const;
    bool isFlowSensorWorking() const;
    unsigned long getTimeSinceLastPulse() const;
};

#endif
