#ifndef BOOT_BUTTON_H
#define BOOT_BUTTON_H

#include <Arduino.h>
#include "config.h"

class BootButtonDetector {
public:
    BootButtonDetector();
    
    // Инициализация детектора
    void begin();
    
    // Обновление состояния (вызывать в loop)
    void update();
    
    // Проверка, нужно ли запустить WiFi сессию
    bool shouldStartWiFiSession();
    
    // Сброс состояния после запуска сессии
    void reset();
    
    // Получение количества нажатий
    int getClickCount() const { return clickCount; }
    
private:
    // Состояние кнопки
    bool lastButtonState;
    bool currentButtonState;
    unsigned long lastDebounceTime;
    unsigned long lastClickTime;
    
    // Счетчик нажатий
    int clickCount;
    bool sessionRequested;
    
    // Методы
    void handleButtonPress();
    void handleButtonRelease();
    void processClickSequence();
};

#endif
