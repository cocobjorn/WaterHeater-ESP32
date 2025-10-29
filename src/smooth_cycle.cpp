#include "smooth_cycle.h"

SmoothCycle::SmoothCycle() 
    : currentState(CYCLE_IDLE)
    , stateStartTime(0)
    , rampUpTime(2000)      // 2 секунды разгон
    , holdTime(3000)        // 3 секунды удержание
    , rampDownTime(2000)     // 2 секунды торможение
    , pauseTime(1000)        // 1 секунда пауза
    , maxPower(80.0)         // 80% максимальная мощность
    , currentPower(0.0)
    , isRunning(false) {
}

void SmoothCycle::setRampUpTime(unsigned long timeMs) {
    rampUpTime = timeMs;
}

void SmoothCycle::setHoldTime(unsigned long timeMs) {
    holdTime = timeMs;
}

void SmoothCycle::setRampDownTime(unsigned long timeMs) {
    rampDownTime = timeMs;
}

void SmoothCycle::setPauseTime(unsigned long timeMs) {
    pauseTime = timeMs;
}

void SmoothCycle::setMaxPower(float power) {
    if (power < 0) power = 0;
    if (power > 100) power = 100;
    maxPower = power;
}

void SmoothCycle::start() {
    currentState = CYCLE_RAMP_UP;
    stateStartTime = millis();
    currentPower = 0.0;
    isRunning = true;
}

void SmoothCycle::stop() {
    currentState = CYCLE_IDLE;
    currentPower = 0.0;
    isRunning = false;
}

void SmoothCycle::reset() {
    stop();
    stateStartTime = 0;
}

void SmoothCycle::update() {
    if (!isRunning) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - stateStartTime;
    
    switch (currentState) {
        case CYCLE_RAMP_UP:
            if (elapsed >= rampUpTime) {
                // Переход к удержанию
                currentState = CYCLE_HOLD;
                stateStartTime = currentTime;
                currentPower = maxPower;
            } else {
                // Плавный разгон
                float progress = (float)elapsed / rampUpTime;
                currentPower = maxPower * progress;
            }
            break;
            
        case CYCLE_HOLD:
            if (elapsed >= holdTime) {
                // Переход к торможению
                currentState = CYCLE_RAMP_DOWN;
                stateStartTime = currentTime;
            }
            currentPower = maxPower;
            break;
            
        case CYCLE_RAMP_DOWN:
            if (elapsed >= rampDownTime) {
                // Переход к паузе
                currentState = CYCLE_PAUSE;
                stateStartTime = currentTime;
                currentPower = 0.0;
            } else {
                // Плавное торможение
                float progress = (float)elapsed / rampDownTime;
                currentPower = maxPower * (1.0 - progress);
            }
            break;
            
        case CYCLE_PAUSE:
            if (elapsed >= pauseTime) {
                // Переход к новому циклу
                currentState = CYCLE_RAMP_UP;
                stateStartTime = currentTime;
                currentPower = 0.0;
            }
            break;
            
        case CYCLE_IDLE:
        default:
            currentPower = 0.0;
            break;
    }
}

SmoothCycle::CycleState SmoothCycle::getState() const {
    return currentState;
}

float SmoothCycle::getCurrentPower() const {
    return currentPower;
}

bool SmoothCycle::isCycleRunning() const {
    return isRunning;
}

float SmoothCycle::getProgress() const {
    if (!isRunning) return 0.0;
    
    unsigned long elapsed = millis() - stateStartTime;
    
    switch (currentState) {
        case CYCLE_RAMP_UP:
            return (float)elapsed / rampUpTime;
        case CYCLE_HOLD:
            return (float)elapsed / holdTime;
        case CYCLE_RAMP_DOWN:
            return (float)elapsed / rampDownTime;
        case CYCLE_PAUSE:
            return (float)elapsed / pauseTime;
        default:
            return 0.0;
    }
}
