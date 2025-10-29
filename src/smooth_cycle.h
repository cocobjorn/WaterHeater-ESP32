#ifndef SMOOTH_CYCLE_H
#define SMOOTH_CYCLE_H

#include <Arduino.h>

class SmoothCycle {
public:
    enum CycleState {
        CYCLE_IDLE,
        CYCLE_RAMP_UP,
        CYCLE_HOLD,
        CYCLE_RAMP_DOWN,
        CYCLE_PAUSE
    };

private:
    CycleState currentState;
    unsigned long stateStartTime;
    
    // Настройки цикла
    unsigned long rampUpTime;     // Время разгона (мс)
    unsigned long holdTime;       // Время удержания (мс)
    unsigned long rampDownTime;   // Время торможения (мс)
    unsigned long pauseTime;       // Пауза между циклами (мс)
    
    float maxPower;               // Максимальная мощность (0-100%)
    float currentPower;            // Текущая мощность
    
    bool isRunning;

public:
    SmoothCycle();
    
    // Настройка параметров цикла
    void setRampUpTime(unsigned long timeMs);
    void setHoldTime(unsigned long timeMs);
    void setRampDownTime(unsigned long timeMs);
    void setPauseTime(unsigned long timeMs);
    void setMaxPower(float power);
    
    // Управление циклом
    void start();
    void stop();
    void reset();
    
    // Обновление (вызывать в loop)
    void update();
    
    // Получение состояния
    CycleState getState() const;
    float getCurrentPower() const;
    bool isCycleRunning() const;
    
    // Получение прогресса (0.0 - 1.0)
    float getProgress() const;
};

#endif
