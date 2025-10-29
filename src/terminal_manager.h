#ifndef TERMINAL_MANAGER_H
#define TERMINAL_MANAGER_H

#include <Arduino.h>
#include "system_state.h"

class TerminalManager {
public:
    // Статические методы для логирования
    static void addLog(const String& message);
    static String getLogs();
    static String processCommand(const String& command, SystemState* state);
    
private:
    static String logBuffer;
    static const int MAX_LOG_SIZE = 2048;
};

#endif
