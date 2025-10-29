#ifndef PHASE_CONTROLLER_H
#define PHASE_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

class PhaseController {
public:
    enum PhaseState {
        PHASE_IDLE,
        PHASE_RUNNING,
        PHASE_ERROR
    };

private:
    // Пины управления
    int zeroCrossPin;
    int triacPins[3];
    bool isInitialized;
    
    // Состояние детектора нуля
    volatile bool lastZeroState;
    volatile bool currentZeroState;
    volatile unsigned long lastZeroCrossTime;
    volatile unsigned long pulseCount;
    
    // Фазовое управление
    PhaseState currentState;
    float targetPower;           // Целевая мощность 0-100%
    float currentPower;          // Текущая мощность 0-100%
    
    // Фазовые смещения (120° = 6667 мкс для 50Гц)
    static const unsigned long PHASE_SHIFT_US_DEFAULT = PHASE_SHIFT_US;
    static const unsigned long HALF_PERIOD_US_DEFAULT = HALF_PERIOD_US;
    static const unsigned long MIN_FIRE_DELAY_US_DEFAULT = MIN_FIRE_DELAY_US;  // Увеличили для безопасности
    static const unsigned long MAX_FIRE_DELAY_US_DEFAULT = MAX_FIRE_DELAY_US; // Уменьшили для безопасности
    static const unsigned long TRIAC_PULSE_US_DEFAULT = TRIAC_PULSE_US; // Длительность импульса включения триака
    
    // Состояние триаков
    bool triacStates[3];
    unsigned long lastFireTimes[3];
    unsigned long triacFireStartTimes[3]; // Время начала импульса для каждого триака
    bool triacFiring[3];                 // Флаг активного импульса
    
    // Фильтрация частоты
    static const int FREQ_FILTER_SAMPLES_DEFAULT = FREQ_FILTER_SAMPLES;
    float frequencyHistory[FREQ_FILTER_SAMPLES];
    int freqHistoryIndex;
    float currentFrequency;
    bool freqHistoryFilled;
    
    // Методы
    void updateZeroCrossDetection();
    void updatePhaseControl();
    void fireTriac(int phase, unsigned long delayUs);
    float calculateFireDelay(float power);
    void updateFrequency();
    float getFilteredFrequency();

public:
    PhaseController();
    
    // Инициализация
    void begin(); // Использует константы из config.h
    void begin(int zeroCrossPin, int triacPin1, int triacPin2, int triacPin3); // Пользовательские пины
    
    // Основной цикл обновления
    void update();
    
    // Управление мощностью (для PID)
    void setTargetPower(float power);  // 0-100%
    float getCurrentPower() const;
    float getTargetPower() const;
    
    // Состояние системы
    PhaseState getState() const;
    bool isReady() const;
    float getFrequency() const;
    
    // Управление
    void start();
    void stop();
    void emergencyStop();
};

#endif
