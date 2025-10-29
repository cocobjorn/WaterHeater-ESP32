#include "phase_controller.h"
#include "config.h"

PhaseController::PhaseController() 
    : zeroCrossPin(0)
    , isInitialized(false)
    , lastZeroState(false)
    , currentZeroState(false)
    , lastZeroCrossTime(0)
    , pulseCount(0)
    , currentState(PHASE_IDLE)
    , targetPower(0.0)
    , currentPower(0.0)
    , freqHistoryIndex(0)
    , currentFrequency(50.0)
    , freqHistoryFilled(false) {
    
    // Инициализация пинов
    for (int i = 0; i < 3; i++) {
        triacPins[i] = 0;
        triacStates[i] = false;
        lastFireTimes[i] = 0;
        triacFireStartTimes[i] = 0;
        triacFiring[i] = false;
    }
    
    // Инициализация фильтра частоты
    for (int i = 0; i < FREQ_FILTER_SAMPLES; i++) {
        frequencyHistory[i] = 50.0;
    }
}

void PhaseController::begin() {
    // Используем константы из config.h
    begin(ZERO_CROSS_PIN, TRIAC_L1_PIN, TRIAC_L2_PIN, TRIAC_L3_PIN);
}

void PhaseController::begin(int zeroCrossPin, int triacPin1, int triacPin2, int triacPin3) {
    this->zeroCrossPin = zeroCrossPin;
    triacPins[0] = triacPin1;
    triacPins[1] = triacPin2;
    triacPins[2] = triacPin3;
    
    // Настройка пина детектора нуля
    pinMode(zeroCrossPin, INPUT_PULLUP);
    lastZeroState = digitalRead(zeroCrossPin);
    currentZeroState = lastZeroState;
    
    // Настройка пинов триаков
    for (int i = 0; i < 3; i++) {
        pinMode(triacPins[i], OUTPUT);
        digitalWrite(triacPins[i], LOW);
    }
    
    isInitialized = true;
    currentState = PHASE_IDLE;
}

void PhaseController::update() {
    if (!isInitialized) return;
    
    updateZeroCrossDetection();
    updatePhaseControl();
}

void PhaseController::updateZeroCrossDetection() {
    currentZeroState = digitalRead(zeroCrossPin);
    
    // Детектируем пересечение нуля (любое изменение состояния)
    if (currentZeroState != lastZeroState) {
        unsigned long currentTime = micros();
        
        // Проверка на дребезг (минимум 100 мкс)
        if (currentTime - lastZeroCrossTime > 100) {
            lastZeroCrossTime = currentTime;
            pulseCount++;
            updateFrequency();
            
            // Отладочная информация о пересечении нуля (каждый 500-й импульс)
            if (pulseCount % 500 == 0) {
                Serial.print("ПЕРЕСЕЧЕНИЕ НУЛЯ #");
                Serial.print(pulseCount);
                Serial.print(" - ");
                Serial.println(currentZeroState ? "HIGH" : "LOW");
            }
        }
    }
    
    // Отладочная информация каждые 5 секунд
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 5000) {
        Serial.println("--- КОНТРОЛЛЕР ФАЗ ---");
        Serial.print("Пин ");
        Serial.print(zeroCrossPin);
        Serial.print(": ");
        Serial.println(currentZeroState ? "HIGH" : "LOW");
        Serial.print("Всего пересечений: ");
        Serial.println(pulseCount);
        Serial.print("Частота: ");
        Serial.print(currentFrequency, 1);
        Serial.println(" Гц");
        Serial.println("---------------------");
        lastDebugTime = millis();
    }
    
    lastZeroState = currentZeroState;
}

void PhaseController::updatePhaseControl() {
    unsigned long currentTime = micros();
    
    // Проверяем завершение импульсов триаков
    for (int phase = 0; phase < 3; phase++) {
        if (triacFiring[phase] && (currentTime - triacFireStartTimes[phase] >= TRIAC_PULSE_US)) {
            // Завершаем импульс
            digitalWrite(triacPins[phase], LOW);
            triacFiring[phase] = false;
        }
    }
    
    if (currentState != PHASE_RUNNING || targetPower < 1.0) {
        // Выключаем все триаки
        for (int i = 0; i < 3; i++) {
            digitalWrite(triacPins[i], LOW);
            triacStates[i] = false;
            triacFiring[i] = false;
        }
        currentPower = 0.0;
        return;
    }
    
    // Плавное изменение текущей мощности с разными скоростями для включения/выключения
    float diff = targetPower - currentPower;
    if (abs(diff) > 0.05) {
        // Разные скорости для включения и выключения
        float rampSpeed = (targetPower > currentPower) ? 0.01 : 0.005; // Медленнее выключаем
        currentPower += diff * rampSpeed;
    } else {
        currentPower = targetPower;
    }
    
    // Ограничиваем мощность
    if (currentPower < 0) currentPower = 0;
    if (currentPower > 100) currentPower = 100;
    
    // Рассчитываем задержку включения
    unsigned long baseDelay = calculateFireDelay(currentPower);
    
    // Управляем каждой фазой с учетом смещения
    for (int phase = 0; phase < 3; phase++) {
        // Рассчитываем смещение для каждой фазы
        unsigned long phaseDelay = baseDelay + (phase * PHASE_SHIFT_US);
        
        // Проверяем, прошло ли время для включения этой фазы
        unsigned long timeSinceZeroCross = currentTime - lastZeroCrossTime;
        
        // Если задержка прошла и мы в пределах полупериода
        if (timeSinceZeroCross >= phaseDelay && timeSinceZeroCross < HALF_PERIOD_US) {
            // Проверяем, не включали ли уже в этом полупериоде
            if (lastFireTimes[phase] < lastZeroCrossTime || lastFireTimes[phase] == 0) {
                fireTriac(phase, phaseDelay);
            }
        }
    }
}

void PhaseController::fireTriac(int phase, unsigned long delayUs) {
    if (phase < 0 || phase >= 3) return;
    
    // Проверяем, что триак еще не включен
    if (triacFiring[phase]) return;
    
    // Отладочная информация о срабатывании триака (только первые 3 включения)
    static int triacFireCount = 0;
    if (triacFireCount < 3) {
        Serial.print("ТРИАК ");
        Serial.print(phase + 1);
        Serial.print(" (пин ");
        Serial.print(triacPins[phase]);
        Serial.print(") ВКЛЮЧЕН, задержка: ");
        Serial.print(delayUs);
        Serial.println(" мкс");
        triacFireCount++;
    }
    
    // Начинаем импульс включения триака
    digitalWrite(triacPins[phase], HIGH);
    triacStates[phase] = true;
    triacFiring[phase] = true;
    triacFireStartTimes[phase] = micros();
    
    // Отмечаем время включения
    lastFireTimes[phase] = micros();
}

float PhaseController::calculateFireDelay(float power) {
    if (power < 1.0) return MAX_FIRE_DELAY_US;
    if (power >= 100.0) return MIN_FIRE_DELAY_US;
    
    // Используем квадратичную зависимость для более плавной работы
    float normalizedPower = power / 100.0;
    float smoothPower = normalizedPower * normalizedPower; // Квадратичная зависимость
    
    // Более безопасный диапазон: 1000мкс - 8500мкс
    return MIN_FIRE_DELAY_US + (MAX_FIRE_DELAY_US - MIN_FIRE_DELAY_US) * (1.0 - smoothPower);
}

void PhaseController::updateFrequency() {
    // Простой расчет частоты на основе количества импульсов за время
    static unsigned long lastFreqUpdate = 0;
    static unsigned long lastPulseCount = 0;
    
    unsigned long currentTime = millis();
    
    // Обновляем частоту каждые 100мс
    if (currentTime - lastFreqUpdate >= 100) {
        unsigned long pulseDiff = pulseCount - lastPulseCount;
        unsigned long timeDiff = currentTime - lastFreqUpdate;
        
        if (timeDiff > 0 && pulseDiff > 0) {
            // Частота = импульсы в секунду / 2 (так как H11AA1 дает импульс на каждое пересечение)
            float frequency = (pulseDiff * 1000.0 / timeDiff) / 2.0;
            
            // Фильтруем разумные значения (40-60 Гц)
            if (frequency >= 40.0 && frequency <= 60.0) {
                frequencyHistory[freqHistoryIndex] = frequency;
                freqHistoryIndex = (freqHistoryIndex + 1) % FREQ_FILTER_SAMPLES;
                
                if (!freqHistoryFilled && freqHistoryIndex == 0) {
                    freqHistoryFilled = true;
                }
                
                currentFrequency = getFilteredFrequency();
            }
        }
        
        lastFreqUpdate = currentTime;
        lastPulseCount = pulseCount;
    }
}

float PhaseController::getFilteredFrequency() {
    if (!freqHistoryFilled) return 50.0;
    
    float sum = 0;
    for (int i = 0; i < FREQ_FILTER_SAMPLES; i++) {
        sum += frequencyHistory[i];
    }
    
    return sum / FREQ_FILTER_SAMPLES;
}

void PhaseController::setTargetPower(float power) {
    if (power < 0) power = 0;
    if (power > 100) power = 100;
    
    targetPower = power;
}

float PhaseController::getCurrentPower() const {
    return currentPower;
}

float PhaseController::getTargetPower() const {
    return targetPower;
}

PhaseController::PhaseState PhaseController::getState() const {
    return currentState;
}

bool PhaseController::isReady() const {
    // Считаем готовым если инициализирован и частота в разумных пределах
    return isInitialized && currentFrequency > 40.0 && currentFrequency < 60.0;
}

float PhaseController::getFrequency() const {
    return currentFrequency;
}

void PhaseController::start() {
    if (!isInitialized) return;
    currentState = PHASE_RUNNING;
}

void PhaseController::stop() {
    currentState = PHASE_IDLE;
    targetPower = 0.0;
    currentPower = 0.0;
    
    // Выключаем все триаки
    for (int i = 0; i < 3; i++) {
        digitalWrite(triacPins[i], LOW);
        triacStates[i] = false;
        triacFiring[i] = false;
    }
}

void PhaseController::emergencyStop() {
    currentState = PHASE_ERROR;
    targetPower = 0.0;
    currentPower = 0.0;
    
    // Немедленно выключаем все триаки
    for (int i = 0; i < 3; i++) {
        digitalWrite(triacPins[i], LOW);
        triacStates[i] = false;
        triacFiring[i] = false;
    }
}