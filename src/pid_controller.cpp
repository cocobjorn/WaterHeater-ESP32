#include "pid_controller.h"

PIDController::PIDController(float kp, float ki, float kd, float outputMin, float outputMax)
    : kp(kp), ki(ki), kd(kd)
    , outputMin(outputMin), outputMax(outputMax)
    , lastError(0.0), integral(0.0), lastTime(0), lastComputeTime(0), lastOutput(0.0)
    , isEnabled(false), setpoint(TARGET_TEMP) // Используем константу из config.h
    , integralMax(PID_INTEGRAL_MAX), integralMin(PID_INTEGRAL_MIN) {
}

float PIDController::compute(float input, float setpoint) {
    if (!isEnabled) return 0.0;
    
    unsigned long currentTime = millis();
    
    // Проверяем интервал вычислений для стабильности
    if (currentTime - lastComputeTime < PID_COMPUTE_INTERVAL_MS) {
        return lastOutput; // Возвращаем последнее значение
    }
    
    float deltaTime = (currentTime - lastTime) / 1000.0; // в секундах
    
    if (deltaTime <= 0) deltaTime = 0.01; // минимум 10мс
    
    float error = setpoint - input;
    
    // Пропорциональная составляющая
    float proportional = kp * error;
    
    // Интегральная составляющая с ограничением
    integral += error * deltaTime;
    
    // Ограничиваем интегральную составляющую
    if (integral > integralMax) integral = integralMax;
    if (integral < integralMin) integral = integralMin;
    
    float integralTerm = ki * integral;
    
    // Дифференциальная составляющая
    float derivative = (error - lastError) / deltaTime;
    float derivativeTerm = kd * derivative;
    
    // Вычисляем выход
    float output = proportional + integralTerm + derivativeTerm;
    
    // Ограничиваем выход
    if (output > outputMax) output = outputMax;
    if (output < outputMin) output = outputMin;
    
    // Сохраняем состояние
    lastError = error;
    lastTime = currentTime;
    lastComputeTime = currentTime;
    lastOutput = output;
    
    return output;
}

void PIDController::setTunings(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PIDController::setOutputLimits(float min, float max) {
    outputMin = min;
    outputMax = max;
    
    // Обновляем ограничения интегральной составляющей
    integralMax = max * 0.1;
    integralMin = min * 0.1;
}

void PIDController::setIntegralLimits(float min, float max) {
    integralMin = min;
    integralMax = max;
}

void PIDController::enable() {
    isEnabled = true;
    lastTime = millis();
}

void PIDController::disable() {
    isEnabled = false;
}

bool PIDController::isControllerEnabled() const {
    return isEnabled;
}

void PIDController::setSetpoint(float setpoint) {
    this->setpoint = setpoint;
}

float PIDController::getSetpoint() const {
    return setpoint;
}

void PIDController::reset() {
    lastError = 0.0;
    integral = 0.0;
    lastTime = millis();
}
