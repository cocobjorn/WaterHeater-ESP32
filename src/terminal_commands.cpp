#include "terminal_commands.h"
#include "system_controller.h"

TerminalCommands::TerminalCommands() : systemController(nullptr), isInitialized(false) {
}

void TerminalCommands::begin(SystemController* controller) {
    systemController = controller;
    isInitialized = true;
    inputBuffer = "";
    
    Serial.println("Терминальные команды инициализированы");
}

void TerminalCommands::update() {
    if (!isInitialized || !systemController) return;
    
    // Читаем данные из Serial
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processCommand(inputBuffer);
                inputBuffer = "";
            }
        } else if (c >= 32 && c <= 126) { // Печатные символы
            inputBuffer += c;
        }
    }
}

void TerminalCommands::processCommand(const String& command) {
    String cmd = command;
    cmd.toLowerCase();
    cmd.trim();
    
    Serial.println("> " + command);
    
    if (cmd == "help" || cmd == "h" || cmd == "?") {
        showHelp();
    } else if (cmd == "status" || cmd == "s") {
        showStatus();
    } else if (cmd == "temp" || cmd == "t") {
        showTemperature();
    } else if (cmd == "flow" || cmd == "f") {
        showFlow();
    } else if (cmd == "enable" || cmd == "on") {
        enableHeating();
    } else if (cmd == "disable" || cmd == "off") {
        disableHeating();
    } else if (cmd == "stop" || cmd == "emergency") {
        emergencyStop();
    } else if (cmd.startsWith("temp ")) {
        setTemperature(cmd.substring(5));
    } else if (cmd.startsWith("flow ")) {
        setFlowRate(cmd.substring(5));
    } else if (cmd == "flowdiag" || cmd == "fd") {
        showFlowDiagnostics();
    } else if (cmd == "testflow") {
        // Тест датчика потока
        Serial.println("=== ТЕСТ ДАТЧИКА ПОТОКА ===");
        Serial.println("Крутите турбинку датчика потока...");
        Serial.println("Ожидаем импульсы на пине 35...");
        Serial.println("Команда 'stop' для выхода из теста");
        
        unsigned long testStartTime = millis();
        float lastFlowRate = 0.0;
        
        while (millis() - testStartTime < 30000) { // 30 секунд теста
            if (Serial.available()) {
                String testCmd = Serial.readString();
                testCmd.trim();
                if (testCmd == "stop") break;
            }
            
            systemController->update();
            
            float currentFlowRate = systemController->getCurrentFlowRate();
            if (abs(currentFlowRate - lastFlowRate) > 0.01) {
                Serial.print("Поток: ");
                Serial.print(currentFlowRate, 2);
                Serial.println(" л/мин");
                lastFlowRate = currentFlowRate;
            }
            
            delay(100);
        }
        Serial.println("Тест завершен");
    } else if (cmd == "reset") {
        systemController->reset();
        Serial.println("Система сброшена");
    } else {
        Serial.println("Неизвестная команда. Введите 'help' для справки.");
    }
}

void TerminalCommands::showHelp() {
    printSeparator();
    Serial.println("=== КОМАНДЫ УПРАВЛЕНИЯ ===");
    Serial.println("help, h, ?     - Показать эту справку");
    Serial.println("status, s      - Статус системы");
    Serial.println("temp, t        - Текущая температура");
    Serial.println("flow, f        - Текущий поток");
    Serial.println("flowdiag, fd   - Диагностика датчика потока");
    Serial.println("enable, on     - Включить нагрев");
    Serial.println("disable, off   - Выключить нагрев");
    Serial.println("stop           - Аварийная остановка");
    Serial.println("reset          - Сброс системы");
    Serial.println("temp <значение> - Установить температуру (40-65°C)");
    Serial.println("flow <значение> - Установить мин. поток (л/мин)");
    Serial.println("testflow       - Тест датчика потока (30 сек)");
    printSeparator();
}

void TerminalCommands::showStatus() {
    printSeparator();
    Serial.println("=== СТАТУС СИСТЕМЫ ===");
    
    String stateName = getStateName(systemController->getState());
    Serial.println("Состояние: " + stateName);
    
    Serial.print("Нагрев: ");
    Serial.println(systemController->isHeatingEnabled() ? "ВКЛ" : "ВЫКЛ");
    
    Serial.print("Аварийная остановка: ");
    Serial.println(systemController->isEmergencyStop() ? "ДА" : "НЕТ");
    
    Serial.print("Температура: ");
    Serial.print(systemController->getCurrentTemperature(), 1);
    Serial.println("°C");
    
    Serial.print("Поток: ");
    Serial.print(systemController->getCurrentFlowRate(), 2);
    Serial.println(" л/мин");
    
    Serial.print("Мощность: ");
    Serial.print(systemController->getCurrentPower(), 1);
    Serial.println("%");
    
    Serial.print("Целевая температура: ");
    Serial.print(systemController->getTargetTemperature(), 1);
    Serial.println("°C");
    
    Serial.print("Мин. поток: ");
    Serial.print(systemController->getMinFlowRate(), 1);
    Serial.println(" л/мин");
    
    printSeparator();
}

void TerminalCommands::showTemperature() {
    float temp = systemController->getCurrentTemperature();
    float target = systemController->getTargetTemperature();
    
    Serial.print("Температура: ");
    Serial.print(temp, 1);
    Serial.print("°C (цель: ");
    Serial.print(target, 1);
    Serial.println("°C)");
}

void TerminalCommands::showFlow() {
    float flow = systemController->getCurrentFlowRate();
    float minFlow = systemController->getMinFlowRate();
    bool flowing = systemController->isWaterFlowing();
    
    Serial.print("Поток: ");
    Serial.print(flow, 2);
    Serial.print(" л/мин (мин: ");
    Serial.print(minFlow, 1);
    Serial.print(" л/мин) - ");
    Serial.println(flowing ? "ЕСТЬ" : "НЕТ");
}

void TerminalCommands::enableHeating() {
    systemController->enableHeating();
    Serial.println("Нагрев ВКЛЮЧЕН");
}

void TerminalCommands::disableHeating() {
    systemController->disableHeating();
    Serial.println("Нагрев ВЫКЛЮЧЕН");
}

void TerminalCommands::emergencyStop() {
    systemController->emergencyStop();
    Serial.println("АВАРИЙНАЯ ОСТАНОВКА!");
}

void TerminalCommands::setTemperature(const String& args) {
    float temp = args.toFloat();
    
    if (temp >= 40.0 && temp <= 65.0) {
        systemController->setTargetTemperature(temp);
        Serial.print("Целевая температура установлена: ");
        Serial.print(temp, 1);
        Serial.println("°C");
  } else {
        Serial.println("Ошибка: температура должна быть от 40 до 65°C");
    }
}

void TerminalCommands::setFlowRate(const String& args) {
    float flow = args.toFloat();
    
    if (flow >= 0.1 && flow <= 10.0) {
        systemController->setMinFlowRate(flow);
        Serial.print("Минимальный поток установлен: ");
        Serial.print(flow, 1);
        Serial.println(" л/мин");
  } else {
        Serial.println("Ошибка: поток должен быть от 0.1 до 10.0 л/мин");
    }
}

void TerminalCommands::showFlowDiagnostics() {
    printSeparator();
    Serial.println("=== ДИАГНОСТИКА ДАТЧИКА ПОТОКА ===");
    
    // Получаем доступ к датчикам через системный контроллер
    const SensorManager& sensors = systemController->getSensors();
    
    // Выводим диагностическую информацию
    sensors.printFlowSensorDiagnostics();
    
    // Дополнительная информация
    Serial.println("=== ДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ ===");
    Serial.print("Датчик работает: ");
    Serial.println(sensors.isFlowSensorWorking() ? "ДА" : "НЕТ");
    Serial.print("Минимальный порог потока: ");
    Serial.print(systemController->getMinFlowRate(), 1);
    Serial.println(" л/мин");
    Serial.print("Поток обнаружен системой: ");
    Serial.println(systemController->isWaterFlowing() ? "ДА" : "НЕТ");
    Serial.println("=====================================");
}

String TerminalCommands::getStateName(int state) {
    switch (state) {
        case 0: return "ПОКОЙ";
        case 1: return "ЗАПУСК";
        case 2: return "НАГРЕВ";
        case 3: return "ПОДДЕРЖАНИЕ";
        case 4: return "ОШИБКА";
        default: return "НЕИЗВЕСТНО";
    }
}

void TerminalCommands::printSeparator() {
    Serial.println("=====================================");
}

SystemController* TerminalCommands::getSystemController() const {
    return systemController;
}