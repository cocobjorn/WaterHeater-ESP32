#ifndef TERMINAL_COMMANDS_H
#define TERMINAL_COMMANDS_H

#include <ArduinoJson.h>
#include "system_state.h"

// Структура для команды терминала
struct TerminalCommand {
  String command;
  String description;
  String (*handler)(const String& args, SystemState* state);
};

// Класс для управления терминалом
class TerminalManager {
public:
  static void begin();
  static String processCommand(const String& input, SystemState* state);
  static void addLog(const String& message);
  static String getLogs();
  static void clearLogs();
  
private:
  static String helpCommand(const String& args, SystemState* state);
  static String statusCommand(const String& args, SystemState* state);
  static String configCommand(const String& args, SystemState* state);
  static String tempCommand(const String& args, SystemState* state);
  static String flowCommand(const String& args, SystemState* state);
  static String heatingCommand(const String& args, SystemState* state);
  static String wifiCommand(const String& args, SystemState* state);
  static String resetCommand(const String& args, SystemState* state);
  static String saveCommand(const String& args, SystemState* state);
  static String logsCommand(const String& args, SystemState* state);
  static String clearCommand(const String& args, SystemState* state);
  static String pinCommand(const String& args, SystemState* state);
  static String sleepCommand(const String& args, SystemState* state);
  static String calibrateCommand(const String& args, SystemState* state);
  static String pidCommand(const String& args, SystemState* state);
  static String sensorCommand(const String& args, SystemState* state);
  static String eepromCommand(const String& args, SystemState* state);
  static String systemCommand(const String& args, SystemState* state);
  static String testCommand(const String& args, SystemState* state);
  
  static String logs[100]; // Буфер логов
  static int logIndex;
  static int logCount;
};

#endif
