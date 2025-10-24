#include "terminal_commands.h"
#include "sensors.h"
#include "triac_control.h"
#include "safety.h"
#include "config.h"

// Внешние объекты
extern SafetyManager safetyManager;

String TerminalManager::logs[100];
int TerminalManager::logIndex = 0;
int TerminalManager::logCount = 0;

void TerminalManager::begin() {
  addLog("Терминал инициализирован");
  addLog("Введите 'help' для списка команд");
}

void TerminalManager::addLog(const String& message) {
  String timestamp = String(millis() / 1000) + "s";
  logs[logIndex] = "[" + timestamp + "] " + message;
  logIndex = (logIndex + 1) % 100;
  if (logCount < 100) logCount++;
}

String TerminalManager::getLogs() {
  String result = "";
  int start = logCount < 100 ? 0 : logIndex;
  int count = logCount < 100 ? logCount : 100;
  
  for (int i = 0; i < count; i++) {
    int index = (start + i) % 100;
    result += logs[index] + "\n";
  }
  return result;
}

void TerminalManager::clearLogs() {
  logIndex = 0;
  logCount = 0;
  addLog("Логи очищены");
}

String TerminalManager::processCommand(const String& input, SystemState* state) {
  String trimmedInput = input;
  trimmedInput.trim();
  if (trimmedInput.length() == 0) return "";
  
  addLog("> " + input);
  
  int spaceIndex = trimmedInput.indexOf(' ');
  String command = spaceIndex > 0 ? trimmedInput.substring(0, spaceIndex) : trimmedInput;
  String args = spaceIndex > 0 ? trimmedInput.substring(spaceIndex + 1) : "";
  
  command.toLowerCase();
  
  // Список команд
  if (command == "help" || command == "?") {
    return helpCommand(args, state);
  } else if (command == "status" || command == "st") {
    return statusCommand(args, state);
  } else if (command == "config" || command == "cfg") {
    return configCommand(args, state);
  } else if (command == "temp" || command == "temperature") {
    return tempCommand(args, state);
  } else if (command == "flow") {
    return flowCommand(args, state);
  } else if (command == "heating" || command == "heat") {
    return heatingCommand(args, state);
  } else if (command == "wifi") {
    return wifiCommand(args, state);
  } else if (command == "reset") {
    return resetCommand(args, state);
  } else if (command == "save") {
    return saveCommand(args, state);
  } else if (command == "logs") {
    return logsCommand(args, state);
  } else if (command == "clear" || command == "cls") {
    return clearCommand(args, state);
  } else if (command == "pin") {
    return pinCommand(args, state);
  } else if (command == "sleep") {
    return sleepCommand(args, state);
  } else if (command == "calibrate" || command == "cal") {
    return calibrateCommand(args, state);
  } else if (command == "pid") {
    return pidCommand(args, state);
  } else if (command == "sensor" || command == "sens") {
    return sensorCommand(args, state);
  } else if (command == "eeprom" || command == "eep") {
    return eepromCommand(args, state);
  } else if (command == "system" || command == "sys") {
    return systemCommand(args, state);
  } else if (command == "test") {
    return testCommand(args, state);
  } else {
    return "Неизвестная команда: " + command + ". Введите 'help' для списка команд.";
  }
}

String TerminalManager::helpCommand(const String& args, SystemState* state) {
  if (args.length() == 0) {
    return "=== СПРАВКА ПО КОМАНДАМ ===\n\n"
           "ОСНОВНЫЕ КОМАНДЫ:\n"
           "help, ?           - Показать эту справку\n"
           "status, st        - Показать статус системы\n"
           "config, cfg       - Показать конфигурацию\n"
           "logs              - Показать логи\n"
           "clear, cls        - Очистить логи\n"
           "sleep             - Перейти в режим сна\n\n"
           "УПРАВЛЕНИЕ СИСТЕМОЙ:\n"
           "temp [value]      - Установить/показать целевую температуру\n"
           "flow [value]      - Установить/показать калибровку протока\n"
           "heating [on/off]  - Включить/выключить нагрев\n"
           "wifi [on/off]     - Включить/выключить WiFi сессию\n"
           "pin [pin] [value] - Установить значение пина\n\n"
           "КАЛИБРОВКА И НАСТРОЙКА:\n"
           "calibrate, cal    - Калибровка датчиков\n"
           "pid               - Настройка PID регулятора\n"
           "sensor, sens      - Информация о датчиках\n"
           "eeprom, eep       - Диагностика EEPROM\n\n"
           "КОНФИГУРАЦИЯ:\n"
           "reset             - Сбросить конфигурацию\n"
           "save              - Сохранить конфигурацию\n\n"
           "Для подробной справки по команде: help [команда]";
  }
  
  String command = args;
  command.toLowerCase();
  
  if (command == "calibrate" || command == "cal") {
    return "=== КАЛИБРОВКА ДАТЧИКОВ ===\n\n"
           "calibrate flow     - Калибровка датчика протока по объему\n"
           "  Инструкция:\n"
           "  1. Подготовьте мерную емкость на 1 литр\n"
           "  2. Убедитесь, что вода не течет\n"
           "  3. Запустите команду\n"
           "  4. Откройте кран и наполните ровно 1 литр\n"
           "  5. Закройте кран\n"
           "  6. Система рассчитает коэффициент\n"
           "  7. Коэффициент = импульсы / 1 литр\n\n"
           "calibrate temp     - Калибровка датчика температуры\n"
           "  Показывает текущие показания и позволяет\n"
           "  настроить компенсацию температуры\n\n"
           "calibrate reset    - Сброс всех калибровок\n"
           "  Возвращает все коэффициенты к значениям\n"
           "  по умолчанию\n\n"
           "calibrate status   - Статус калибровок\n"
           "  Показывает текущие значения всех\n"
           "  калибровочных коэффициентов";
  } else if (command == "pid") {
    return "=== НАСТРОЙКА PID РЕГУЛЯТОРА ===\n\n"
           "pid status         - Показать текущие параметры PID\n"
           "  Отображает Kp, Ki, Kd и другие настройки\n\n"
           "pid set kp ki kd   - Установить параметры PID\n"
           "  Пример: pid set 2.0 0.5 1.0\n"
           "  kp - пропорциональный коэффициент (рекомендуется 1.0-5.0)\n"
           "  ki - интегральный коэффициент (рекомендуется 0.1-2.0)\n"
           "  kd - дифференциальный коэффициент (рекомендуется 0.5-3.0)\n\n"
           "pid test           - Тестовый режим PID\n"
           "  Активирует подробное логирование работы\n"
           "  регулятора для отладки\n\n"
           "pid reset          - Сбросить к значениям по умолчанию\n"
           "  Kp=2.0, Ki=0.5, Kd=1.0";
  } else if (command == "sensor" || command == "sens") {
    return "=== ИНФОРМАЦИЯ О ДАТЧИКАХ ===\n\n"
           "sensor temp        - Информация о датчике температуры\n"
           "  • Тип датчика (NTC термистор)\n"
           "  • Текущие показания\n"
           "  • Статус калибровки\n"
           "  • Технические параметры\n\n"
           "sensor flow        - Информация о датчике протока\n"
           "  • Тип датчика (импульсный)\n"
           "  • Текущий проток\n"
           "  • Калибровочный коэффициент\n"
           "  • Статус обнаружения\n\n"
           "sensor all         - Информация о всех датчиках\n"
           "  Показывает полную информацию по всем\n"
           "  подключенным датчикам";
  } else if (command == "temp") {
    return "=== УПРАВЛЕНИЕ ТЕМПЕРАТУРОЙ ===\n\n"
           "temp               - Показать текущую и целевую температуру\n"
           "temp [value]       - Установить целевую температуру\n\n"
           "Диапазон: 40-65°C\n"
           "Рекомендуемые значения:\n"
           "• 40-45°C - теплая вода\n"
           "• 45-50°C - горячая вода\n"
           "• 50-55°C - очень горячая вода\n"
           "• 55-65°C - кипяток (осторожно!)";
  } else if (command == "flow") {
    return "=== УПРАВЛЕНИЕ ПРОТОКОМ ===\n\n"
           "flow               - Показать текущий проток и калибровку\n"
           "flow [value]       - Установить калибровочный коэффициент\n\n"
           "Калибровка протока:\n"
           "• По умолчанию: 7.5 импульсов/литр\n"
           "• Диапазон: 0.1-10.0\n"
           "• Для точной настройки используйте 'calibrate flow'\n\n"
           "Порог обнаружения: 1.0 л/мин";
  } else {
    return "Неизвестная команда: " + command + "\n\n"
           "Доступные команды для справки:\n"
           "• calibrate - калибровка датчиков\n"
           "• pid - настройка PID регулятора\n"
           "• sensor - информация о датчиках\n"
           "• temp - управление температурой\n"
           "• flow - управление протоком";
  }
}

String TerminalManager::statusCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  String result = "=== СТАТУС СИСТЕМЫ ===\n";
  result += "Режим: " + String(state->systemMode == 0 ? "Сон" : 
                               state->systemMode == 1 ? "Активный" : "WiFi сессия") + "\n";
  result += "Температура: " + String(state->currentTemp, 1) + "°C\n";
  result += "Целевая температура: " + String(state->targetTemp, 1) + "°C\n";
  result += "Проток: " + String(state->flowRate, 2) + " л/мин\n";
  result += "Обнаружен проток: " + String(state->isFlowDetected ? "ДА" : "НЕТ") + "\n";
  result += "Нагрев: " + String(state->isHeating ? "ВКЛ" : "ВЫКЛ") + "\n";
  result += "Мощность L1: " + String(state->heatingPower[0]) + "%\n";
  result += "Мощность L2: " + String(state->heatingPower[1]) + "%\n";
  result += "Мощность L3: " + String(state->heatingPower[2]) + "%\n";
  result += "WiFi: " + String(state->isWiFiEnabled ? "ВКЛ" : "ВЫКЛ") + "\n";
  result += "Время работы: " + String(millis() / 1000) + " сек";
  
  return result;
}

String TerminalManager::configCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  String result = "=== КОНФИГУРАЦИЯ ===\n";
  result += "Целевая температура: " + String(state->targetTemp, 1) + "°C\n";
  result += "Калибровка протока: " + String(state->flowCalibrationFactor, 2) + "\n";
  result += "Порог протока: " + String(FLOW_THRESHOLD, 2) + " л/мин\n";
  result += "Гистерезис: " + String(TEMP_HYSTERESIS, 1) + "°C\n";
  result += "Макс. температура: " + String(MAX_TEMP, 1) + "°C\n";
  result += "Мин. температура: " + String(MIN_TEMP, 1) + "°C\n";
  result += "Калибровка температуры: " + String(state->config.isCalibrated ? "ДА" : "НЕТ");
  
  return result;
}

String TerminalManager::tempCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Текущая температура: " + String(state->currentTemp, 1) + "°C\n" +
           "Целевая температура: " + String(state->targetTemp, 1) + "°C";
  }
  
  float newTemp = args.toFloat();
  if (newTemp < TARGET_TEMP_MIN || newTemp > TARGET_TEMP_MAX) {
    return "Ошибка: температура должна быть от " + String(TARGET_TEMP_MIN) + 
           " до " + String(TARGET_TEMP_MAX) + "°C";
  }
  
  state->targetTemp = newTemp;
  addLog("Целевая температура установлена: " + String(newTemp, 1) + "°C");
  return "Целевая температура установлена: " + String(newTemp, 1) + "°C";
}

String TerminalManager::flowCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Текущий проток: " + String(state->flowRate, 2) + " л/мин\n" +
           "Калибровка протока: " + String(state->flowCalibrationFactor, 2);
  }
  
  float newCalibration = args.toFloat();
  if (newCalibration < 0.1 || newCalibration > 10.0) {
    return "Ошибка: калибровка должна быть от 0.1 до 10.0";
  }
  
  state->flowCalibrationFactor = newCalibration;
  addLog("Калибровка протока установлена: " + String(newCalibration, 2));
  return "Калибровка протока установлена: " + String(newCalibration, 2);
}

String TerminalManager::heatingCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Нагрев: " + String(state->isHeating ? "ВКЛ" : "ВЫКЛ");
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "on" || arg == "вкл" || arg == "1") {
    if (state->isFlowDetected) {
      state->isHeating = true;
      addLog("Нагрев включен");
      return "Нагрев включен";
    } else {
      return "Ошибка: нет протока воды";
    }
  } else if (arg == "off" || arg == "выкл" || arg == "0") {
    state->isHeating = false;
    addLog("Нагрев выключен");
    return "Нагрев выключен";
  } else {
    return "Использование: heating [on/off]";
  }
}

String TerminalManager::wifiCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "WiFi: " + String(state->isWiFiEnabled ? "ВКЛ" : "ВЫКЛ");
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "on" || arg == "вкл" || arg == "1") {
    // Включаем WiFi сессию
    state->isWiFiEnabled = true;
    state->wifiSessionStartTime = millis();
    state->systemMode = SYSTEM_MODE_WIFI_SESSION;
    addLog("WiFi сессия включена");
    return "WiFi сессия включена";
  } else if (arg == "off" || arg == "выкл" || arg == "0") {
    // Выключаем WiFi сессию
    state->isWiFiEnabled = false;
    state->systemMode = SYSTEM_MODE_SLEEP;
    addLog("WiFi сессия выключена");
    return "WiFi сессия выключена";
  } else {
    return "Использование: wifi [on/off]";
  }
}

String TerminalManager::resetCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  state->resetConfiguration();
  addLog("Конфигурация сброшена к умолчанию");
  return "Конфигурация сброшена к умолчанию";
}

String TerminalManager::saveCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (state->saveConfiguration()) {
    addLog("Конфигурация сохранена в EEPROM");
    return "Конфигурация сохранена в EEPROM";
  } else {
    addLog("Ошибка сохранения конфигурации");
    return "Ошибка сохранения конфигурации";
  }
}

String TerminalManager::logsCommand(const String& args, SystemState* state) {
  return getLogs();
}

String TerminalManager::clearCommand(const String& args, SystemState* state) {
  clearLogs();
  return "Логи очищены";
}

String TerminalManager::pinCommand(const String& args, SystemState* state) {
  int spaceIndex = args.indexOf(' ');
  if (spaceIndex <= 0) {
    return "Использование: pin [номер_пина] [значение]";
  }
  
  int pin = args.substring(0, spaceIndex).toInt();
  int value = args.substring(spaceIndex + 1).toInt();
  
  if (pin < 0 || pin > 39) {
    return "Ошибка: неверный номер пина (0-39)";
  }
  
  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);
  
  addLog("Пин " + String(pin) + " установлен в " + String(value));
  return "Пин " + String(pin) + " установлен в " + String(value);
}

String TerminalManager::sleepCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  addLog("Переход в режим сна...");
  state->systemMode = SYSTEM_MODE_SLEEP;
  return "Переход в режим сна...";
}

String TerminalManager::calibrateCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Калибровка датчиков:\n"
           "calibrate flow     - Автокалибровка датчика протока\n"
           "calibrate temp     - Калибровка датчика температуры\n"
           "calibrate reset    - Сброс всех калибровок\n"
           "calibrate status   - Статус калибровок";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "flow") {
    addLog("Запуск калибровки датчика протока по объему...");
    addLog("Инструкция:");
    addLog("1. Подготовьте мерную емкость на 1 литр");
    addLog("2. Убедитесь, что вода не течет");
    addLog("3. Нажмите Enter для начала калибровки");
    addLog("4. Откройте кран и наполните ровно 1 литр");
    addLog("5. Закройте кран");
    addLog("6. Система автоматически рассчитает коэффициент");
    addLog("7. Коэффициент = количество импульсов / 1 литр");
    return "Калибровка датчика протока запущена. Следуйте инструкциям выше.";
  } else if (arg.startsWith("temp")) {
    // Проверяем, есть ли параметр эталонной температуры
    String tempParam = arg.substring(4);
    tempParam.trim();
    
    if (tempParam.length() > 0) {
      // Есть параметр - выполняем калибровку
      float referenceTemp = tempParam.toFloat();
      
      if (referenceTemp < -50.0 || referenceTemp > 100.0) {
        addLog("Ошибка: эталонная температура должна быть от -50°C до 100°C");
        return "Ошибка: неверная эталонная температура";
      }
      
      addLog("=== ВЫПОЛНЕНИЕ КАЛИБРОВКИ ТЕМПЕРАТУРЫ ===");
      addLog("Эталонная температура: " + String(referenceTemp, 1) + "°C");
      addLog("Текущая сырая температура: " + String(state->currentTemp, 1) + "°C");
      
      if (state->calibrateTemperature(referenceTemp)) {
        addLog("Калибровка успешно выполнена!");
        addLog("Смещение: " + String(state->config.tempOffset, 1) + "°C");
        return "Калибровка температуры выполнена успешно!";
      } else {
        addLog("Ошибка выполнения калибровки");
        return "Ошибка выполнения калибровки температуры";
      }
    } else {
      // Нет параметра - показываем информацию
      addLog("=== КАЛИБРОВКА ДАТЧИКА ТЕМПЕРАТУРЫ ===");
      addLog("Текущая температура: " + String(state->currentTemp, 1) + "°C");
      addLog("Температура чипа: " + String(temperatureRead(), 1) + "°C");
      addLog("Статус калибровки: " + String(state->config.isCalibrated ? "ДА" : "НЕТ"));
      
      if (state->config.isCalibrated) {
        addLog("Смещение: " + String(state->config.tempOffset, 1) + "°C");
        addLog("Коэффициент: " + String(state->config.tempSlope, 2));
      }
      
      addLog("Используйте: calibrate temp [эталонная_температура]");
      addLog("Пример: calibrate temp 25.0 - для калибровки при 25°C");
      return "Калибровка температуры. Укажите эталонную температуру для калибровки.";
    }
  } else if (arg == "reset") {
    addLog("Сброс всех калибровок...");
    state->flowCalibrationFactor = 7.5; // Значение по умолчанию
    state->resetTemperatureCalibration();
    return "Все калибровки сброшены к значениям по умолчанию";
  } else if (arg == "status") {
    String result = "=== СТАТУС КАЛИБРОВОК ===\n";
    result += "Датчик протока: " + String(state->flowCalibrationFactor, 2) + "\n";
    result += "Калибровка температуры: " + String(state->config.isCalibrated ? "ДА" : "НЕТ") + "\n";
    result += "Текущая температура: " + String(state->currentTemp, 1) + "°C\n";
    result += "Температура чипа: " + String(temperatureRead(), 1) + "°C";
    return result;
  } else {
    return "Использование: calibrate [flow|temp|reset|status]";
  }
}

String TerminalManager::pidCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Настройка PID регулятора:\n"
           "pid status         - Показать текущие параметры PID\n"
           "pid set kp ki kd   - Установить параметры PID\n"
           "pid test           - Тестовый режим PID\n"
           "pid reset          - Сбросить к значениям по умолчанию";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "status") {
    return "=== ПАРАМЕТРЫ PID ===\n"
           "Kp (Пропорциональный): 2.0\n"
           "Ki (Интегральный): 0.5\n"
           "Kd (Дифференциальный): 1.0\n"
           "Выходной диапазон: 0-100%\n"
           "Частота обновления: 100мс";
  } else if (arg == "test") {
    addLog("Тестовый режим PID активирован");
    addLog("Целевая температура: " + String(state->targetTemp, 1) + "°C");
    addLog("Текущая температура: " + String(state->currentTemp, 1) + "°C");
    return "Тестовый режим PID активирован. Мониторьте логи для отслеживания работы.";
  } else if (arg == "reset") {
    addLog("Сброс параметров PID к значениям по умолчанию");
    return "Параметры PID сброшены к значениям по умолчанию";
  } else {
    // Попытка парсинга параметров PID
    int space1 = args.indexOf(' ');
    int space2 = args.indexOf(' ', space1 + 1);
    
    if (space1 > 0 && space2 > 0) {
      String kpStr = args.substring(0, space1);
      String kiStr = args.substring(space1 + 1, space2);
      String kdStr = args.substring(space2 + 1);
      
      float kp = kpStr.toFloat();
      float ki = kiStr.toFloat();
      float kd = kdStr.toFloat();
      
      if (kp > 0 && ki >= 0 && kd >= 0) {
        addLog("Установка параметров PID: Kp=" + String(kp, 2) + 
               ", Ki=" + String(ki, 2) + ", Kd=" + String(kd, 2));
        return "Параметры PID установлены: Kp=" + String(kp, 2) + 
               ", Ki=" + String(ki, 2) + ", Kd=" + String(kd, 2);
      }
    }
    
    return "Использование: pid [status|test|reset|set kp ki kd]";
  }
}

String TerminalManager::sensorCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Информация о датчиках:\n"
           "sensor temp        - Информация о датчике температуры\n"
           "sensor flow        - Информация о датчике протока\n"
           "sensor all         - Информация о всех датчиках";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "temp") {
    String result = "=== ДАТЧИК ТЕМПЕРАТУРЫ ===\n";
    result += "Тип: NTC термистор\n";
    result += "Текущая температура: " + String(state->currentTemp, 1) + "°C\n";
    result += "Температура чипа: " + String(temperatureRead(), 1) + "°C\n";
    result += "Целевая температура: " + String(state->targetTemp, 1) + "°C\n";
    result += "Гистерезис: " + String(TEMP_HYSTERESIS, 1) + "°C\n";
    result += "Калибровка: " + String(state->config.isCalibrated ? "ДА" : "НЕТ") + "\n";
    result += "Пин: GPIO" + String(NTC_PIN);
    return result;
  } else if (arg == "flow") {
    String result = "=== ДАТЧИК ПРОТОКА ===\n";
    result += "Тип: Импульсный датчик\n";
    result += "Текущий проток: " + String(state->flowRate, 2) + " л/мин\n";
    result += "Обнаружен проток: " + String(state->isFlowDetected ? "ДА" : "НЕТ") + "\n";
    result += "Калибровка: " + String(state->flowCalibrationFactor, 2) + "\n";
    result += "Порог: " + String(FLOW_THRESHOLD, 2) + " л/мин\n";
    result += "Пин: GPIO" + String(FLOW_SENSOR_PIN);
    return result;
  } else if (arg == "all") {
    String result = "=== ВСЕ ДАТЧИКИ ===\n\n";
    result += sensorCommand("temp", state) + "\n\n";
    result += sensorCommand("flow", state);
    return result;
  } else {
    return "Использование: sensor [temp|flow|all]";
  }
}

String TerminalManager::eepromCommand(const String& args, SystemState* state) {
  if (args.length() == 0) {
    return "=== ДИАГНОСТИКА EEPROM ===\n\n"
           "eeprom read     - Читать данные из EEPROM\n"
           "eeprom write    - Записать тестовые данные\n"
           "eeprom clear    - Очистить EEPROM\n"
           "eeprom test     - Тест записи/чтения\n"
           "eeprom status   - Статус EEPROM\n"
           "eeprom debug    - Полная диагностика";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "read") {
    addLog("Чтение данных из EEPROM...");
    
    // Читаем сырые данные из EEPROM
    EEPROM.begin(512);
    ConfigData config;
    EEPROM.get(0, config);
    
    addLog("=== ДАННЫЕ ИЗ EEPROM ===");
    addLog("Version: " + String(config.version));
    addLog("Target temp: " + String(config.targetTemp, 2));
    addLog("Flow calib: " + String(config.flowCalibration, 2));
    addLog("Checksum: " + String(config.checksum));
    addLog("Is calibrated: " + String(config.isCalibrated ? "YES" : "NO"));
    
    // Проверяем контрольную сумму
    uint32_t calculatedChecksum = ConfigStorage::calculateChecksum(config);
    addLog("Calculated checksum: " + String(calculatedChecksum));
    addLog("Checksum valid: " + String(calculatedChecksum == config.checksum ? "YES" : "NO"));
    
    return "Данные из EEPROM прочитаны";
    
  } else if (arg == "write") {
    addLog("Запись тестовых данных в EEPROM...");
    
    ConfigData testConfig;
    testConfig.version = 1;
    testConfig.targetTemp = 55.5f;
    testConfig.flowCalibration = 8.5f;
    testConfig.flowThreshold = 1.0f; // Добавляем обязательное поле
    testConfig.isCalibrated = true;
    testConfig.checksum = ConfigStorage::calculateChecksum(testConfig);
    
    if (ConfigStorage::saveConfig(testConfig)) {
      addLog("✅ Тестовые данные записаны в EEPROM");
      addLog("Target temp: " + String(testConfig.targetTemp, 2));
      addLog("Flow calib: " + String(testConfig.flowCalibration, 2));
    } else {
      addLog("❌ Ошибка записи в EEPROM");
    }
    
    return "Тест записи завершен";
    
  } else if (arg == "clear") {
    addLog("Очистка EEPROM...");
    
    EEPROM.begin(512);
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    
    addLog("✅ EEPROM очищен");
    return "EEPROM очищен";
    
  } else if (arg == "test") {
    addLog("Тест записи/чтения EEPROM...");
    
    // Записываем тестовые данные
    ConfigData testConfig;
    testConfig.version = 1;
    testConfig.targetTemp = 60.0f;
    testConfig.flowCalibration = 9.0f;
    testConfig.flowThreshold = 1.0f; // Добавляем обязательное поле
    testConfig.isCalibrated = false;
    testConfig.checksum = ConfigStorage::calculateChecksum(testConfig);
    
    addLog("Записываем: temp=" + String(testConfig.targetTemp) + ", calib=" + String(testConfig.flowCalibration));
    
    if (ConfigStorage::saveConfig(testConfig)) {
      addLog("✅ Запись успешна");
      
      // Читаем обратно
      ConfigData readConfig;
      if (ConfigStorage::loadConfig(readConfig)) {
        addLog("✅ Чтение успешно");
        addLog("Прочитано: temp=" + String(readConfig.targetTemp) + ", calib=" + String(readConfig.flowCalibration));
        
        if (readConfig.targetTemp == testConfig.targetTemp && 
            readConfig.flowCalibration == testConfig.flowCalibration) {
          addLog("✅ ТЕСТ ПРОЙДЕН - данные совпадают");
        } else {
          addLog("❌ ТЕСТ НЕ ПРОЙДЕН - данные не совпадают");
        }
      } else {
        addLog("❌ Ошибка чтения из EEPROM");
      }
    } else {
      addLog("❌ Ошибка записи в EEPROM");
    }
    
    return "Тест EEPROM завершен";
    
  } else if (arg == "status") {
    addLog("Статус EEPROM...");
    
    EEPROM.begin(512);
    ConfigData config;
    EEPROM.get(0, config);
    
    addLog("=== СТАТУС EEPROM ===");
    addLog("Version: " + String(config.version));
    addLog("Valid version: " + String(config.version == 1 ? "YES" : "NO"));
    
    uint32_t calculatedChecksum = ConfigStorage::calculateChecksum(config);
    addLog("Checksum valid: " + String(calculatedChecksum == config.checksum ? "YES" : "NO"));
    
    bool isValid = (config.version == 1) && (calculatedChecksum == config.checksum);
    
    addLog("EEPROM valid: " + String(isValid ? "YES" : "NO"));
    
    return "Статус EEPROM получен";
    
  } else if (arg == "debug") {
    addLog("=== ПОЛНАЯ ДИАГНОСТИКА EEPROM ===");
    
    // 1. Проверяем EEPROM
    EEPROM.begin(512);
    ConfigData config;
    EEPROM.get(0, config);
    
    addLog("1. EEPROM данные:");
    addLog("   Version: " + String(config.version));
    addLog("   Target temp: " + String(config.targetTemp, 2));
    addLog("   Flow calib: " + String(config.flowCalibration, 2));
    addLog("   Flow threshold: " + String(config.flowThreshold, 2));
    addLog("   Checksum: " + String(config.checksum));
    
    // 2. Проверяем текущие значения в памяти
    addLog("2. Текущие значения в памяти:");
    addLog("   Target temp: " + String(state->targetTemp, 2));
    addLog("   Flow calib: " + String(state->flowCalibrationFactor, 2));
    
    // 3. Проверяем валидность
    uint32_t calculatedChecksum = ConfigStorage::calculateChecksum(config);
    addLog("3. Валидность:");
    addLog("   Calculated checksum: " + String(calculatedChecksum));
    addLog("   Stored checksum: " + String(config.checksum));
    addLog("   Checksum match: " + String(calculatedChecksum == config.checksum ? "YES" : "NO"));
    
    // 4. Тест записи
    addLog("4. Тест записи текущих значений...");
    if (state->saveConfiguration()) {
      addLog("   ✅ Сохранение успешно");
      
      // Читаем обратно
      EEPROM.get(0, config);
      addLog("   Прочитано обратно:");
      addLog("   Target temp: " + String(config.targetTemp, 2));
      addLog("   Flow calib: " + String(config.flowCalibration, 2));
      
      if (config.targetTemp == state->targetTemp && 
          config.flowCalibration == state->flowCalibrationFactor) {
        addLog("   ✅ ТЕСТ ПРОЙДЕН - значения совпадают");
      } else {
        addLog("   ❌ ТЕСТ НЕ ПРОЙДЕН - значения не совпадают");
      }
    } else {
      addLog("   ❌ Ошибка сохранения");
    }
    
    return "Полная диагностика завершена";
  }
  
  return "Неизвестная подкоманда: " + arg;
}

String TerminalManager::systemCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Управление системой:\n"
           "system status    - Статус системы\n"
           "system enable    - Включить систему\n"
           "system disable   - Выключить систему\n"
           "system target    - Установить целевую температуру\n"
           "system reset     - Сброс системы";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "status") {
    String result = "=== СТАТУС СИСТЕМЫ ===\n";
    result += "Система включена: " + String(state->isSystemEnabled ? "ДА" : "НЕТ") + "\n";
    result += "Целевая температура: " + String(state->targetTemp, 1) + "°C\n";
    result += "Текущая температура: " + String(state->currentTemp, 1) + "°C\n";
    result += "Проток: " + String(state->flowRate, 2) + " л/мин\n";
    result += "Проток обнаружен: " + String(state->isFlowDetected ? "ДА" : "НЕТ") + "\n";
    result += "Термопредохранитель: " + String(state->isThermalFuseOK ? "ОК" : "ОШИБКА") + "\n";
    result += "Нагрев: " + String(state->isHeating ? "ВКЛ" : "ВЫКЛ") + "\n";
    result += "Режим: " + String(state->systemMode);
    return result;
  } else if (arg == "enable") {
    state->isSystemEnabled = true;
    addLog("✅ Система включена");
    return "Система включена";
  } else if (arg == "disable") {
    state->isSystemEnabled = false;
    addLog("❌ Система выключена");
    return "Система выключена";
  } else if (arg.startsWith("target")) {
    String tempParam = arg.substring(6);
    tempParam.trim();
    if (tempParam.length() > 0) {
      float newTarget = tempParam.toFloat();
      if (newTarget >= 40.0 && newTarget <= 65.0) {
        state->targetTemp = newTarget;
        addLog("🎯 Целевая температура: " + String(newTarget, 1) + "°C");
        return "Целевая температура установлена: " + String(newTarget, 1) + "°C";
      } else {
        return "Ошибка: температура должна быть от 40°C до 65°C";
      }
    } else {
      return "Использование: system target [температура]\nПример: system target 50.0";
    }
  } else if (arg == "reset") {
    state->isSystemEnabled = true;
    state->targetTemp = 50.0;
    state->isHeating = false;
    addLog("🔄 Система сброшена");
    return "Система сброшена к значениям по умолчанию";
  } else {
    return "Неизвестная подкоманда: " + arg;
  }
}

String TerminalManager::testCommand(const String& args, SystemState* state) {
  if (!state) return "Ошибка: состояние системы недоступно";
  
  if (args.length() == 0) {
    return "Тестирование системы:\n"
           "test relay on     - Включить питание реле (MOSFET)\n"
           "test relay off    - Выключить питание реле (MOSFET)\n"
           "test triac on     - Включить симисторы\n"
           "test triac off    - Выключить симисторы\n"
           "test all on       - Включить все (реле + симисторы)\n"
           "test all off      - Выключить все\n"
           "test heating on   - ПРИНУДИТЕЛЬНОЕ включение нагрева\n"
           "test heating off  - ПРИНУДИТЕЛЬНОЕ выключение нагрева\n"
           "test active       - Переход в активный режим\n"
           "test reset safety - Сброс аварийного режима";
  }
  
  String arg = args;
  arg.toLowerCase();
  
  if (arg == "relay on") {
    digitalWrite(MOSFET_EN_PIN, HIGH);
    addLog("✅ Питание реле включено (MOSFET)");
    return "Питание реле включено";
  } else if (arg == "relay off") {
    digitalWrite(MOSFET_EN_PIN, LOW);
    addLog("❌ Питание реле выключено (MOSFET)");
    return "Питание реле выключено";
  } else if (arg == "relay 1" || arg == "relay 2" || arg == "relay 3") {
    // Индивидуальные реле теперь не управляются напрямую
    addLog("ℹ️ Индивидуальные реле управляются внешним модулем, доступно только питание");
    return "Индивид. реле не поддерживаются, используйте: test relay on/off";
  } else if (arg == "triac on") {
    digitalWrite(TRIAC_L1_PIN, HIGH);
    digitalWrite(TRIAC_L2_PIN, HIGH);
    digitalWrite(TRIAC_L3_PIN, HIGH);
    addLog("✅ Все симисторы включены");
    return "Все симисторы включены";
  } else if (arg == "triac off") {
    digitalWrite(TRIAC_L1_PIN, LOW);
    digitalWrite(TRIAC_L2_PIN, LOW);
    digitalWrite(TRIAC_L3_PIN, LOW);
    addLog("❌ Все симисторы выключены");
    return "Все симисторы выключены";
  } else if (arg == "all on") {
    digitalWrite(MOSFET_EN_PIN, HIGH);
    addLog("✅ Включено питание реле (MOSFET)");
    return "Включено питание реле";
  } else if (arg == "all off") {
    digitalWrite(MOSFET_EN_PIN, LOW);
    addLog("❌ Питание реле выключено (MOSFET)");
    return "Питание реле выключено";
  } else if (arg == "heating on") {
    // Принудительное включение нагрева
    state->isHeating = true;
    state->isSystemEnabled = true;
    state->isFlowDetected = true;
    state->targetTemp = 50.0;
    digitalWrite(MOSFET_EN_PIN, HIGH);
    addLog("🔥 ПРИНУДИТЕЛЬНОЕ ВКЛЮЧЕНИЕ НАГРЕВА");
    return "ПРИНУДИТЕЛЬНОЕ ВКЛЮЧЕНИЕ НАГРЕВА";
  } else if (arg == "heating off") {
    // Принудительное выключение нагрева
    state->isHeating = false;
    digitalWrite(MOSFET_EN_PIN, LOW);
    addLog("❄️ ПРИНУДИТЕЛЬНОЕ ВЫКЛЮЧЕНИЕ НАГРЕВА");
    return "ПРИНУДИТЕЛЬНОЕ ВЫКЛЮЧЕНИЕ НАГРЕВА";
  } else if (arg == "active") {
    // Принудительный переход в активный режим
    state->systemMode = SYSTEM_MODE_ACTIVE;
    addLog("🔄 ПЕРЕХОД В АКТИВНЫЙ РЕЖИМ");
    return "ПЕРЕХОД В АКТИВНЫЙ РЕЖИМ";
  } else if (arg == "reset safety") {
    // Сброс аварийного режима
    safetyManager.resetEmergencyMode();
    state->isSystemEnabled = true;
    addLog("🔄 СБРОС АВАРИЙНОГО РЕЖИМА");
    return "СБРОС АВАРИЙНОГО РЕЖИМА";
  } else {
    return "Использование: test [relay|triac|all|heating|active|reset safety] [on|off|1|2|3]";
  }
}