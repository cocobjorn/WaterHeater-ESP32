#include "system_controller.h"

SystemController::SystemController() : 
    currentState(STATE_IDLE),
    previousState(STATE_IDLE),
    stateStartTime(0),
    lastUpdateTime(0),
    minFlowRate(MIN_FLOW_RATE_DEFAULT),           // 0.5 л/мин по умолчанию
    targetTemperature(TARGET_TEMP_DEFAULT),
    minTemperature(MIN_TEMP_DEFAULT),       // Минимум 40°C
    maxTemperature(MAX_TEMP_DEFAULT),       // Максимум 65°C
    rampUpTime(RAMP_UP_TIME_MS),           // 2 секунды разгона
    currentTargetPower(0.0),
    rampStartPower(0.0),
    rampTargetPower(0.0),
    heatingEnabled(true),
    flowDetected(false),
    currentFlowRate(0.0),
    currentTemperature(25.0),
    emergencyStopFlag(false),
    lastFlowCheckTime(0) {
}

void SystemController::begin() {
    // Инициализируем компоненты
    sensors.begin();
    phaseController.begin();
    
    // Настраиваем PID контроллер
    pidController.setSetpoint(targetTemperature);
    pidController.setOutputLimits(0.0, 100.0);
    pidController.enable();
    
    // Настраиваем контроллер фаз
    phaseController.start();
    
    // Переходим в режим покоя
    transitionToState(STATE_IDLE);
    
    lastUpdateTime = millis();
}

void SystemController::update() {
    unsigned long currentTime = millis();
    
    // Обновляем датчики
    updateSensors();
    
    // Проверяем защитные условия
    if (!checkSafetyConditions()) {
        emergencyShutdown();
        return;
    }
    
    // Обновляем машину состояний
    updateStateMachine();
    
    // Обновляем контроллер фаз
    phaseController.update();
    
    lastUpdateTime = currentTime;
}

void SystemController::updateSensors() {
    sensors.update();
    
    currentFlowRate = sensors.getFlowRate();
    currentTemperature = sensors.getTemperature();
    
    // Обновляем состояние потока
    bool newFlowDetected = (currentFlowRate >= minFlowRate);
    
    // Отладочная информация каждые 2 секунды
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 2000) {
        Serial.println("=== СТАТУС СИСТЕМЫ ===");
        Serial.print("Поток: ");
        Serial.print(currentFlowRate, 2);
        Serial.print(" л/мин (мин: ");
        Serial.print(minFlowRate, 1);
        Serial.print(" л/мин) - ");
        Serial.println(newFlowDetected ? "ОБНАРУЖЕН" : "НЕТ");
        
        Serial.print("Температура: ");
        Serial.print(currentTemperature, 1);
        Serial.print("°C (цель: ");
        Serial.print(targetTemperature, 1);
        Serial.println("°C)");
        
        Serial.print("Нагрев: ");
        Serial.print(heatingEnabled ? "ВКЛ" : "ВЫКЛ");
        Serial.print(", Состояние: ");
        Serial.print(getState());
        Serial.print(", Мощность: ");
        Serial.print(currentTargetPower, 1);
        Serial.println("%");
        
        // Отладочная информация ПИД-регулятора
        if (pidController.isControllerEnabled()) {
            float temperatureError = targetTemperature - currentTemperature;
            Serial.print("ПИД: Ошибка=");
            Serial.print(temperatureError, 2);
            Serial.print("°C, ПИД-выход=");
            Serial.print(pidController.getLastOutput(), 1);
            Serial.print("%, Логика=");
            if (temperatureError > 5.0) {
                Serial.print("100% (далеко)");
            } else if (temperatureError > 2.0) {
                Serial.print("80%+ (приближение)");
            } else {
                Serial.print("60% (поддержание)");
            }
            Serial.println();
        }
        
        Serial.println("========================");
        lastDebugTime = millis();
    }
    
    // Если поток только что появился или исчез
    if (newFlowDetected != flowDetected) {
        // Добавляем гистерезис для стабилизации потока
        static unsigned long lastFlowChangeTime = 0;
        unsigned long currentTime = millis();
        
        // Минимальный интервал между изменениями потока (200мс)
        if (currentTime - lastFlowChangeTime < 200) {
            return; // Игнорируем слишком частые изменения
        }
        
        lastFlowChangeTime = currentTime;
        flowDetected = newFlowDetected;
        
        Serial.print("Изменение потока: ");
        Serial.println(flowDetected ? "ПОЯВИЛСЯ" : "ИСЧЕЗ");
        
        if (flowDetected && heatingEnabled) {
            // Поток появился - начинаем нагрев
            Serial.println("Запуск нагрева...");
            transitionToState(STATE_STARTING);
        } else if (!flowDetected) {
            // Поток исчез - останавливаем нагрев
            Serial.println("Остановка нагрева...");
            transitionToState(STATE_IDLE);
        }
    }
}

void SystemController::updateStateMachine() {
    switch (currentState) {
        case STATE_IDLE:
            handleIdleState();
            break;
        case STATE_STARTING:
            handleStartingState();
            break;
        case STATE_HEATING:
            handleHeatingState();
            break;
        case STATE_COOLING_DOWN:
            handleCoolingDownState();
            break;
        case STATE_ERROR:
            handleErrorState();
            break;
    }
}

void SystemController::handleIdleState() {
    // В режиме покоя только мониторим датчики
    // Нагрев отключен
    phaseController.setTargetPower(0.0);
    currentTargetPower = 0.0;
    
    // Если поток появился и нагрев включен - переходим к запуску
    if (flowDetected && heatingEnabled) {
        transitionToState(STATE_STARTING);
    }
}

void SystemController::handleStartingState() {
    // Простой разгон до 100% мощности
    updateRampUp();
    
    // Если разгон завершен - переходим к активному нагреву
    if (currentTargetPower >= rampTargetPower) {
        // Сбрасываем PID перед переходом к активному нагреву
        pidController.reset();
        transitionToState(STATE_HEATING);
    }
}

void SystemController::handleHeatingState() {
    // Активный нагрев с PID регулированием
    if (flowDetected && heatingEnabled) {
        // Рассчитываем мощность через PID
        float pidOutput = pidController.compute(currentTemperature, targetTemperature);
        
        // Простая логика: если температура далека от целевой - работаем на максимум
        float temperatureError = targetTemperature - currentTemperature;
        
        float finalPower;
        if (temperatureError > 5.0) {
            // Температура далека от целевой - работаем на 100%
            finalPower = 100.0;
        } else if (temperatureError > 2.0) {
            // Приближаемся к целевой - используем ПИД, но не менее 80%
            finalPower = max(pidOutput, 80.0f);
        } else {
            // Близко к целевой - переходим к поддержанию
            transitionToState(STATE_COOLING_DOWN);
            return;
        }
        
        phaseController.setTargetPower(finalPower);
        currentTargetPower = finalPower;
    } else {
        // Поток исчез или нагрев отключен
        transitionToState(STATE_IDLE);
    }
}

void SystemController::handleCoolingDownState() {
    // Поддержание температуры с PID регулированием
    if (flowDetected && heatingEnabled) {
        // Рассчитываем мощность через PID
        float pidOutput = pidController.compute(currentTemperature, targetTemperature);
        
        // Ограничиваем мощность для поддержания (максимум 60%)
        float finalPower = min(pidOutput, 60.0f);
        
        phaseController.setTargetPower(finalPower);
        currentTargetPower = finalPower;
        
        // Если температура упала значительно - возвращаемся к активному нагреву
        float temperatureError = targetTemperature - currentTemperature;
        if (temperatureError > 3.0) {
            transitionToState(STATE_HEATING);
        }
    } else {
        // Поток исчез или нагрев отключен
        transitionToState(STATE_IDLE);
    }
}

void SystemController::handleErrorState() {
    // В режиме ошибки отключаем все
    phaseController.setTargetPower(0.0);
    phaseController.emergencyStop();
    currentTargetPower = 0.0;
}

void SystemController::transitionToState(SystemState newState) {
    if (newState != currentState) {
        previousState = currentState;
        currentState = newState;
        stateStartTime = millis();
        
        // Действия при переходе в новое состояние
        switch (newState) {
            case STATE_STARTING:
                // Начинаем плавный разгон
                startRampUp(100.0); // Разгон до 100% мощности
                break;
                
            case STATE_IDLE:
                // Сбрасываем PID при остановке
                pidController.reset();
                break;
                
            case STATE_ERROR:
                // Аварийная остановка
                phaseController.emergencyStop();
                break;
                
            default:
                break;
        }
    }
}

void SystemController::startRampUp(float targetPower) {
    rampStartPower = currentTargetPower;
    rampTargetPower = targetPower;
}

void SystemController::updateRampUp() {
    unsigned long elapsed = millis() - stateStartTime;
    
    if (elapsed >= rampUpTime) {
        // Разгон завершен
        currentTargetPower = rampTargetPower;
    } else {
        // Линейный разгон
        float progress = (float)elapsed / rampUpTime;
        currentTargetPower = rampStartPower + (rampTargetPower - rampStartPower) * progress;
    }
    
    phaseController.setTargetPower(currentTargetPower);
}

bool SystemController::checkSafetyConditions() {
    // Проверяем критические условия
    if (currentTemperature > maxTemperature + 5.0) {
        return false; // Перегрев
    }
    
    if (currentTemperature < -5.0) {
        return false; // Некорректные показания датчика
    }
    
    if (emergencyStopFlag) {
        return false; // Аварийная остановка
    }
    
    return true;
}

void SystemController::emergencyShutdown() {
    transitionToState(STATE_ERROR);
    emergencyStopFlag = true;
}

// ========================================
// PUBLIC METHODS
// ========================================

void SystemController::enableHeating() {
    heatingEnabled = true;
}

void SystemController::disableHeating() {
    heatingEnabled = false;
    transitionToState(STATE_IDLE);
}

void SystemController::emergencyStop() {
    emergencyStopFlag = true;
    emergencyShutdown();
}

void SystemController::reset() {
    emergencyStopFlag = false;
    heatingEnabled = false;
    transitionToState(STATE_IDLE);
    pidController.reset();
}

void SystemController::setMinFlowRate(float flowRate) {
    minFlowRate = flowRate;
}

void SystemController::setTargetTemperature(float temperature) {
    if (temperature >= minTemperature && temperature <= maxTemperature) {
        targetTemperature = temperature;
        pidController.setSetpoint(temperature);
    }
}

void SystemController::setTemperatureRange(float minTemp, float maxTemp) {
    minTemperature = minTemp;
    maxTemperature = maxTemp;
    
    // Корректируем целевую температуру если нужно
    if (targetTemperature < minTemp) {
        targetTemperature = minTemp;
    } else if (targetTemperature > maxTemp) {
        targetTemperature = maxTemp;
    }
    
    pidController.setSetpoint(targetTemperature);
}

void SystemController::setRampUpTime(unsigned long timeMs) {
    rampUpTime = timeMs;
}

SystemController::SystemState SystemController::getState() const {
    return currentState;
}

float SystemController::getCurrentFlowRate() const {
    return currentFlowRate;
}

float SystemController::getCurrentTemperature() const {
    return currentTemperature;
}

float SystemController::getCurrentPower() const {
    return currentTargetPower;
}

bool SystemController::isHeatingEnabled() const {
    return heatingEnabled;
}

bool SystemController::isEmergencyStop() const {
    return emergencyStopFlag;
}

bool SystemController::isWaterFlowing() const {
    return flowDetected;
}

float SystemController::getMinFlowRate() const {
    return minFlowRate;
}

float SystemController::getTargetTemperature() const {
    return targetTemperature;
}

float SystemController::getMinTemperature() const {
    return minTemperature;
}

float SystemController::getMaxTemperature() const {
    return maxTemperature;
}

// Методы для работы с датчиками
SensorManager& SystemController::getSensors() {
    return sensors;
}

const SensorManager& SystemController::getSensors() const {
    return sensors;
}
