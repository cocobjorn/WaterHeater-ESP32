#ifndef TERMINAL_COMMANDS_H
#define TERMINAL_COMMANDS_H

#include <Arduino.h>

// Предварительное объявление класса SystemController
class SystemController;

class TerminalCommands {
private:
    SystemController* systemController;
    String inputBuffer;
    bool isInitialized;
    
    // Методы обработки команд
    void processCommand(const String& command);
    void showHelp();
    void showStatus();
    void showTemperature();
    void showFlow();
    void enableHeating();
    void disableHeating();
    void emergencyStop();
    void setTemperature(const String& args);
    void setFlowRate(const String& args);
    void showFlowDiagnostics();
    
    // Вспомогательные методы
    String getStateName(int state);
    void printSeparator();

public:
    TerminalCommands();
    
    void begin(SystemController* controller);
    void update();
    
    // Методы для получения данных
    SystemController* getSystemController() const;
};

#endif