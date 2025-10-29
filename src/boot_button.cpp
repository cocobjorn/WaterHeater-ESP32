#include "boot_button.h"

BootButtonDetector::BootButtonDetector() 
    : lastButtonState(HIGH)  // Кнопка BOOT подтянута к HIGH
    , currentButtonState(HIGH)
    , lastDebounceTime(0)
    , lastClickTime(0)
    , clickCount(0)
    , sessionRequested(false) {
}

void BootButtonDetector::begin() {
    // Настройка пина кнопки BOOT (встроенная кнопка)
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    // Инициализация состояния
    lastButtonState = digitalRead(BOOT_BUTTON_PIN);
    currentButtonState = lastButtonState;
    
    if (DEBUG_SERIAL) {
        Serial.println("Детектор кнопки BOOT инициализирован");
        Serial.println("Тройное нажатие активирует WiFi сессию на 15 минут");
    }
}

void BootButtonDetector::update() {
    // Читаем текущее состояние кнопки
    int reading = digitalRead(BOOT_BUTTON_PIN);
    
    // Защита от дребезга
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    // Если состояние стабильно достаточно долго
    if ((millis() - lastDebounceTime) > BOOT_BUTTON_DEBOUNCE_MS) {
        // Если состояние кнопки изменилось
        if (reading != currentButtonState) {
            currentButtonState = reading;
            
            if (currentButtonState == LOW) {
                // Кнопка нажата
                handleButtonPress();
            } else {
                // Кнопка отпущена
                handleButtonRelease();
            }
        }
    }
    
    // Обновляем предыдущее состояние
    lastButtonState = reading;
    
    // Проверяем таймаут между нажатиями
    if (clickCount > 0 && (millis() - lastClickTime) > BOOT_BUTTON_CLICK_TIMEOUT_MS) {
        if (DEBUG_SERIAL) {
            Serial.println("Таймаут нажатий истек, сброс счетчика");
        }
        clickCount = 0;
    }
}

void BootButtonDetector::handleButtonPress() {
    // Кнопка нажата - ничего особенного не делаем
    if (DEBUG_SERIAL) {
        Serial.println("Кнопка BOOT нажата");
    }
}

void BootButtonDetector::handleButtonRelease() {
    // Кнопка отпущена - регистрируем нажатие
    unsigned long currentTime = millis();
    
    // Проверяем, не слишком ли быстрое нажатие
    if ((currentTime - lastClickTime) > BOOT_BUTTON_DEBOUNCE_MS) {
        clickCount++;
        lastClickTime = currentTime;
        
        if (DEBUG_SERIAL) {
            Serial.print("Нажатие #");
            Serial.println(clickCount);
        }
        
        // Проверяем последовательность нажатий
        processClickSequence();
    }
}

void BootButtonDetector::processClickSequence() {
    if (clickCount >= BOOT_BUTTON_CLICKS_REQUIRED) {
        sessionRequested = true;
        
        if (DEBUG_SERIAL) {
            Serial.println("=== ТРОЙНОЕ НАЖАТИЕ ОБНАРУЖЕНО ===");
            Serial.println("Запрос на запуск WiFi сессии");
            Serial.println("================================");
        }
        
        // Сбрасываем счетчик после успешного тройного нажатия
        clickCount = 0;
    }
}

bool BootButtonDetector::shouldStartWiFiSession() {
    if (sessionRequested) {
        sessionRequested = false; // Сбрасываем флаг после проверки
        return true;
    }
    return false;
}

void BootButtonDetector::reset() {
    clickCount = 0;
    sessionRequested = false;
    lastClickTime = 0;
    
    if (DEBUG_SERIAL) {
        Serial.println("Детектор кнопки BOOT сброшен");
    }
}
