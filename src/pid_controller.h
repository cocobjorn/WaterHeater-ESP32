#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

class PIDController {
private:
    // Параметры PID
    float kp, ki, kd;
    float outputMin, outputMax;
    
    // Состояние регулятора
    float lastError;
    float integral;
    unsigned long lastTime;
    unsigned long lastComputeTime;
    float lastOutput;
    
    // Настройки
    bool isEnabled;
    float setpoint;
    
    // Ограничения интегральной составляющей
    float integralMax;
    float integralMin;

public:
    PIDController(float kp = PID_KP, float ki = PID_KI, float kd = PID_KD, 
                  float outputMin = PID_OUTPUT_MIN, float outputMax = PID_OUTPUT_MAX);
    
    // Основные методы
    float compute(float input, float setpoint);
    void setTunings(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setIntegralLimits(float min, float max);
    
    // Управление
    void enable();
    void disable();
    bool isControllerEnabled() const;
    
    // Настройки
    void setSetpoint(float setpoint);
    float getSetpoint() const;
    
    // Сброс
    void reset();
    
    // Получение параметров
    float getKp() const { return kp; }
    float getKi() const { return ki; }
    float getKd() const { return kd; }
    
    // Отладочная информация
    float getLastError() const { return lastError; }
    float getIntegral() const { return integral; }
    float getLastOutput() const { return lastOutput; }
};

#endif
