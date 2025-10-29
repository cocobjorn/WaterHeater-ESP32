#include "terminal_manager.h"

String TerminalManager::logBuffer = "";

void TerminalManager::addLog(const String& message) {
    // Добавляем сообщение в буфер логов
    logBuffer += String(millis() / 1000) + "s: " + message + "\n";
    
    // Ограничиваем размер буфера
    if (logBuffer.length() > MAX_LOG_SIZE) {
        int newlinePos = logBuffer.indexOf('\n', logBuffer.length() - MAX_LOG_SIZE);
        if (newlinePos > 0) {
            logBuffer = logBuffer.substring(newlinePos + 1);
        }
    }
    
    // Также выводим в Serial для отладки
    if (DEBUG_SERIAL) {
        Serial.println(message);
    }
}

String TerminalManager::getLogs() {
    return logBuffer;
}

String TerminalManager::processCommand(const String& command, SystemState* state) {
    String result = "";
    
    if (command == "help") {
        result = "Доступные команды:\n";
        result += "help - показать справку\n";
        result += "status - показать состояние системы\n";
        result += "temp <value> - установить целевую температуру\n";
        result += "calibrate - калибровать датчик потока\n";
        result += "reset - сбросить конфигурацию\n";
    }
    else if (command == "status") {
        if (state) {
            result = "Состояние системы:\n";
            result += "Температура: " + String(state->currentTemp, 1) + "°C\n";
            result += "Целевая температура: " + String(state->targetTemp, 1) + "°C\n";
            result += "Поток: " + String(state->flowRate, 2) + " л/мин\n";
            result += "Нагрев: " + String(state->isHeating ? "ВКЛ" : "ВЫКЛ") + "\n";
            result += "WiFi: " + String(state->isWiFiEnabled ? "ВКЛ" : "ВЫКЛ") + "\n";
        } else {
            result = "Состояние системы недоступно";
        }
    }
    else if (command.startsWith("temp ")) {
        if (state) {
            float temp = command.substring(5).toFloat();
            if (temp >= TARGET_TEMP_MIN && temp <= TARGET_TEMP_MAX) {
                state->targetTemp = temp;
                state->saveConfiguration();
                result = "Целевая температура установлена: " + String(temp, 1) + "°C";
            } else {
                result = "Неверная температура. Диапазон: " + String(TARGET_TEMP_MIN) + "-" + String(TARGET_TEMP_MAX) + "°C";
            }
        } else {
            result = "Система не инициализирована";
        }
    }
    else if (command == "calibrate") {
        result = "Калибровка датчика потока запущена";
    }
    else if (command == "reset") {
        if (state) {
            state->resetConfiguration();
            result = "Конфигурация сброшена к настройкам по умолчанию";
        } else {
            result = "Система не инициализирована";
        }
    }
    else {
        result = "Неизвестная команда: " + command + "\nВведите 'help' для справки";
    }
    
    return result;
}
