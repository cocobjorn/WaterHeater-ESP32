#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "sensors.h"
#include "phase_controller.h"
#include "pid_controller.h"
#include "config.h"

class SystemController {
public:
    enum SystemState {
        STATE_IDLE,           // Режим покоя - только мониторинг датчиков
        STATE_STARTING,       // Плавный запуск нагрева
        STATE_HEATING,        // Активный нагрев с PID
        STATE_COOLING_DOWN,   // Плавное снижение мощности
        STATE_ERROR           // Ошибка системы
    };

private:
    // Компоненты системы
    SensorManager sensors;
    PhaseController phaseController;
    PIDController pidController;
    
    // Состояние системы
    SystemState currentState;
    SystemState previousState;
    unsigned long stateStartTime;
    unsigned long lastUpdateTime;
    
    // Настройки системы
    float minFlowRate;        // Минимальный поток для включения (л/мин)
    float targetTemperature;  // Целевая температура (°C)
    float minTemperature;     // Минимальная температура (°C)
    float maxTemperature;     // Максимальная температура (°C)
    
    // Плавный запуск
    unsigned long rampUpTime; // Время разгона до полной мощности (мс)
    float currentTargetPower; // Текущая целевая мощность
    float rampStartPower;     // Мощность в начале разгона
    float rampTargetPower;    // Целевая мощность разгона
    
    // Состояние нагрева
    bool heatingEnabled;
    bool flowDetected;
    float currentFlowRate;
    float currentTemperature;
    
    // Защитные функции
    bool emergencyStopFlag;
    unsigned long lastFlowCheckTime;
    static const unsigned long FLOW_CHECK_INTERVAL_MS = 500;
    
    // Методы
    void updateSensors();
    void updateStateMachine();
    void handleIdleState();
    void handleStartingState();
    void handleHeatingState();
    void handleCoolingDownState();
    void handleErrorState();
    
    void transitionToState(SystemState newState);
    void startRampUp(float targetPower);
    void updateRampUp();
    
    // Защитные функции
    bool checkSafetyConditions();
    void emergencyShutdown();

public:
    SystemController();
    
    // Инициализация
    void begin();
    
    // Основной цикл
    void update();
    
    // Управление системой
    void enableHeating();
    void disableHeating();
    void emergencyStop();
    void reset();
    
    // Настройки
    void setMinFlowRate(float flowRate);
    void setTargetTemperature(float temperature);
    void setTemperatureRange(float minTemp, float maxTemp);
    void setRampUpTime(unsigned long timeMs);
    
    // Получение состояния
    SystemState getState() const;
    float getCurrentFlowRate() const;
    float getCurrentTemperature() const;
    float getCurrentPower() const;
    bool isHeatingEnabled() const;
    bool isEmergencyStop() const;
    bool isWaterFlowing() const;
    
    // Получение настроек
    float getMinFlowRate() const;
    float getTargetTemperature() const;
    float getMinTemperature() const;
    float getMaxTemperature() const;
    
    // Методы для работы с датчиками
    SensorManager& getSensors();
    const SensorManager& getSensors() const;
};

#endif
