/*
 * СИСТЕМА УПРАВЛЕНИЯ ПРОТОЧНЫМ ВОДОНАГРЕВАТЕЛЕМ
 * Полная система с управлением потоком и температурой
 * 
 * ОСНОВНЫЕ ФУНКЦИИ:
 * - Автоматическое включение при потоке >0.5л/мин
 * - Плавный разгон за 2 секунды
 * - PID регулирование температуры 40-65°C
 * - Режим покоя для экономии ресурсов
 * - Защитные функции
 */

#include "system_controller.h"
#include "web_server.h"
#include "config_storage.h"
#include "boot_button.h"
#include "sensors.h"
#include "config.h"

SystemController systemController;
WebServerManager webServer;
SystemState systemState;
BootButtonDetector bootButton;

void setup() {
    // Инициализация последовательного порта
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    Serial.println("=== ПРОТОЧНЫЙ ВОДОНАГРЕВАТЕЛЬ ===");
    Serial.println("Инициализация системы...");
    
    // Инициализация EEPROM для хранения конфигурации
    ConfigStorage::begin();
    
    // Загрузка конфигурации из EEPROM
    if (!systemState.loadConfiguration()) {
        Serial.println("Используем настройки по умолчанию");
    }
    
    // Инициализация системы управления
    systemController.begin();
    
    // Настройка параметров системы из загруженной конфигурации
    systemController.setMinFlowRate(MIN_FLOW_RATE_DEFAULT);        // Минимальный поток 0.5 л/мин
    systemController.setTargetTemperature(systemState.targetTemp); // Целевая температура из конфигурации
    systemController.setTemperatureRange(MIN_TEMP_DEFAULT, MAX_TEMP_DEFAULT); // Диапазон 40-65°C
    systemController.setRampUpTime(RAMP_UP_TIME_MS);      // Разгон за 2 секунды
    
    // Инициализация веб-сервера
    webServer.begin();
    webServer.setSystemController(&systemController);
    
    // Инициализация детектора кнопки BOOT
    bootButton.begin();
    
    Serial.println("Система готова к работе!");
    Serial.println("Мониторинг данных в реальном времени...");
    Serial.println("=====================================");
}

void loop() {
    // Обновляем систему управления
    systemController.update();
    
    // Обновляем детектор кнопки BOOT
    bootButton.update();
    
    // Проверяем, нужно ли запустить WiFi сессию
    if (bootButton.shouldStartWiFiSession()) {
        webServer.startWiFiSession();
        bootButton.reset();
    }
    
    // Обновляем состояние системы для веб-интерфейса
    updateSystemState();
    
    // Обновляем веб-сервер (только если сессия активна)
    webServer.handleClient();
    
    // Простой тест детектора нуля каждые 10 секунд
    static unsigned long lastTestTime = 0;
    if (millis() - lastTestTime > 10000) {
        Serial.println("=== ТЕСТ ДЕТЕКТОРА НУЛЯ ===");
        Serial.print("Пин 15: ");
        Serial.println(digitalRead(ZERO_CROSS_PIN) ? "HIGH" : "LOW");
        Serial.print("Пин 21: ");
        Serial.println(digitalRead(TRIAC_L1_PIN) ? "HIGH" : "LOW");
        Serial.print("Пин 19: ");
        Serial.println(digitalRead(TRIAC_L2_PIN) ? "HIGH" : "LOW");
        Serial.print("Пин 18: ");
        Serial.println(digitalRead(TRIAC_L3_PIN) ? "HIGH" : "LOW");
        Serial.println("==========================");
        
        // Дополнительная диагностика датчика потока
        Serial.println("=== ДАТЧИК ПОТОКА ===");
        Serial.print("Пин 35: ");
        Serial.println(digitalRead(FLOW_SENSOR_PIN) ? "HIGH" : "LOW");
        Serial.print("Поток: ");
        Serial.print(systemController.getCurrentFlowRate(), 2);
        Serial.print(" л/мин, Обнаружен: ");
        Serial.println(systemController.isWaterFlowing() ? "ДА" : "НЕТ");
        Serial.println("=====================");
        
        // Расширенная диагностика датчика протока
        Serial.println("--- ДАТЧИК ПОТОКА ---");
        Serial.print("Импульсы: ");
        Serial.print(systemController.getSensors().getFlowPulseCount());
        Serial.print(", Последний импульс: ");
        Serial.print(systemController.getSensors().getTimeSinceLastPulse());
        Serial.println(" мс назад");
        Serial.print("Сырое значение пина: ");
        Serial.println(digitalRead(FLOW_SENSOR_PIN) ? "HIGH" : "LOW");
        Serial.print("Скорость потока: ");
        Serial.print(systemController.getCurrentFlowRate(), 2);
        Serial.print(" л/мин, Обнаружен: ");
        Serial.println(systemController.isWaterFlowing() ? "ДА" : "НЕТ");
        Serial.println("--------------------");
        
        lastTestTime = millis();
    }
    
    // Убираем задержку для максимальной скорости чтения детектора
    // delay(10);
}

void updateSystemState() {
    // Обновляем состояние системы для веб-интерфейса
    systemState.currentTemp = systemController.getCurrentTemperature();
    systemState.targetTemp = systemController.getTargetTemperature();
    systemState.flowRate = systemController.getCurrentFlowRate();
    systemState.isHeating = systemController.isHeatingEnabled();
    systemState.isFlowDetected = systemController.isWaterFlowing();
    systemState.isSystemEnabled = !systemController.isEmergencyStop();
    systemState.currentTargetPower = systemController.getCurrentPower();
    
    // Обновляем состояние WiFi сессии
    systemState.isWiFiEnabled = webServer.isWiFiSessionActive();
    systemState.wifiSessionStartTime = webServer.isWiFiSessionActive() ? 
        (millis() - webServer.getSessionTimeLeft()) : 0;
    systemState.systemMode = webServer.isWiFiSessionActive() ? 
        SYSTEM_MODE_WIFI_SESSION : SYSTEM_MODE_ACTIVE;
    
    // Обновляем состояние в веб-сервере
    webServer.updateStatus(systemState);
}
