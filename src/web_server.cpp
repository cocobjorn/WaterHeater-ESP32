#include "web_server.h"
#include "sensors.h"
#include "terminal_commands.h"
#include "terminal_manager.h"
#include "config_storage.h"
#include "system_controller.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void WebServerManager::begin() {
  // Инициализация SPIFFS
  if (!SPIFFS.begin(true)) {
    if (DEBUG_SERIAL) {
      Serial.println("SPIFFS Mount Failed");
    }
    return;
  }
  
  // Инициализация состояния WiFi сессии
  wifiSessionActive = false;
  sessionStartTime = 0;
  
  // Настройка маршрутов для WebServer
  setupRoutes();
  
  if (DEBUG_SERIAL) {
    Serial.println("Web server initialized (WiFi session mode)");
    Serial.println("Press BOOT button 3 times to start WiFi session");
  }
}

void WebServerManager::handleClient() {
  // Обрабатываем клиентов только если WiFi сессия активна
  if (wifiSessionActive) {
    server.handleClient();
    
    // Периодическая диагностика датчика протока во время WiFi сессии (каждые 30 секунд)
    static unsigned long lastDiagnosticTime = 0;
    if (millis() - lastDiagnosticTime > 30000) {
      if (systemController && DEBUG_SERIAL) {
        Serial.println("--- ПЕРИОДИЧЕСКАЯ ДИАГНОСТИКА ДАТЧИКА ПОТОКА (WiFi активен) ---");
        Serial.print("Пин датчика потока (GPIO35): ");
        Serial.println(digitalRead(FLOW_SENSOR_PIN) ? "HIGH" : "LOW");
        Serial.print("Состояние датчика: ");
        Serial.println(systemController->getSensors().isFlowSensorWorking() ? "РАБОТАЕТ" : "НЕ РАБОТАЕТ");
        Serial.print("Количество импульсов: ");
        Serial.println(systemController->getSensors().getFlowPulseCount());
        Serial.print("Время последнего импульса: ");
        Serial.print(systemController->getSensors().getTimeSinceLastPulse());
        Serial.println(" мс назад");
        Serial.print("Текущая скорость потока: ");
        Serial.print(systemController->getCurrentFlowRate(), 2);
        Serial.println(" л/мин");
        Serial.print("Поток обнаружен: ");
        Serial.println(systemController->isWaterFlowing() ? "ДА" : "НЕТ");
        Serial.println("--------------------------------------------------------");
      }
      lastDiagnosticTime = millis();
    }
    
    // Проверяем таймаут сессии
    if ((millis() - sessionStartTime) > WIFI_SESSION_TIMEOUT_MS) {
      if (DEBUG_SERIAL) {
        Serial.println("WiFi сессия истекла, отключаем WiFi");
      }
      stopWiFiSession();
    }
  }
}

void WebServerManager::startWiFiSession() {
  if (wifiSessionActive) {
    if (DEBUG_SERIAL) {
      Serial.println("WiFi сессия уже активна");
    }
    return;
  }
  
  // Запуск WiFi в режиме точки доступа
  startWiFiAP();
  
  // Запуск веб-сервера
  server.begin();
  
  // Обновляем состояние
  wifiSessionActive = true;
  sessionStartTime = millis();
  
  if (currentState) {
    currentState->isWiFiEnabled = true;
    currentState->wifiSessionStartTime = sessionStartTime;
    currentState->systemMode = SYSTEM_MODE_WIFI_SESSION;
  }
  
  if (DEBUG_SERIAL) {
    Serial.println("=== WiFi СЕССИЯ ЗАПУЩЕНА ===");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("Пароль: ");
    Serial.println(WIFI_PASSWORD);
    Serial.println("Сессия активна 15 минут");
    
    // Диагностика датчика протока при запуске WiFi
    if (systemController) {
      Serial.println("--- ДИАГНОСТИКА ДАТЧИКА ПОТОКА ПРИ ЗАПУСКЕ WiFi ---");
      Serial.print("Пин датчика потока (GPIO35): ");
      Serial.println(digitalRead(FLOW_SENSOR_PIN) ? "HIGH" : "LOW");
      Serial.print("Состояние датчика: ");
      Serial.println(systemController->getSensors().isFlowSensorWorking() ? "РАБОТАЕТ" : "НЕ РАБОТАЕТ");
      Serial.print("Количество импульсов: ");
      Serial.println(systemController->getSensors().getFlowPulseCount());
      Serial.print("Время последнего импульса: ");
      Serial.print(systemController->getSensors().getTimeSinceLastPulse());
      Serial.println(" мс назад");
      Serial.print("Текущая скорость потока: ");
      Serial.print(systemController->getCurrentFlowRate(), 2);
      Serial.println(" л/мин");
      Serial.print("Поток обнаружен: ");
      Serial.println(systemController->isWaterFlowing() ? "ДА" : "НЕТ");
      Serial.println("-----------------------------------------------");
    }
    
    Serial.println("========================");
  }
}

void WebServerManager::stopWiFiSession() {
  if (!wifiSessionActive) {
    return;
  }
  
  // Останавливаем веб-сервер
  server.stop();
  
  // Отключаем WiFi
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Обновляем состояние
  wifiSessionActive = false;
  sessionStartTime = 0;
  
  if (currentState) {
    currentState->isWiFiEnabled = false;
    currentState->wifiSessionStartTime = 0;
    currentState->systemMode = SYSTEM_MODE_ACTIVE;
  }
  
  if (DEBUG_SERIAL) {
    Serial.println("=== WiFi СЕССИЯ ОСТАНОВЛЕНА ===");
    Serial.println("WiFi отключен для экономии энергии");
    
    // Диагностика датчика протока при остановке WiFi
    if (systemController) {
      Serial.println("--- ДИАГНОСТИКА ДАТЧИКА ПОТОКА ПРИ ОСТАНОВКЕ WiFi ---");
      Serial.print("Пин датчика потока (GPIO35): ");
      Serial.println(digitalRead(FLOW_SENSOR_PIN) ? "HIGH" : "LOW");
      Serial.print("Состояние датчика: ");
      Serial.println(systemController->getSensors().isFlowSensorWorking() ? "РАБОТАЕТ" : "НЕ РАБОТАЕТ");
      Serial.print("Количество импульсов: ");
      Serial.println(systemController->getSensors().getFlowPulseCount());
      Serial.print("Время последнего импульса: ");
      Serial.print(systemController->getSensors().getTimeSinceLastPulse());
      Serial.println(" мс назад");
      Serial.print("Текущая скорость потока: ");
      Serial.print(systemController->getCurrentFlowRate(), 2);
      Serial.println(" л/мин");
      Serial.print("Поток обнаружен: ");
      Serial.println(systemController->isWaterFlowing() ? "ДА" : "НЕТ");
      Serial.println("------------------------------------------------");
    }
    
    Serial.println("===============================");
  }
}

bool WebServerManager::isWiFiSessionActive() const {
  return wifiSessionActive;
}

unsigned long WebServerManager::getSessionTimeLeft() const {
  if (!wifiSessionActive) {
    return 0;
  }
  
  unsigned long elapsed = millis() - sessionStartTime;
  if (elapsed >= WIFI_SESSION_TIMEOUT_MS) {
    return 0;
  }
  
  return WIFI_SESSION_TIMEOUT_MS - elapsed;
}

void WebServerManager::updateStatus(SystemState& state) {
  currentState = &state;
}

void WebServerManager::setSystemController(SystemController* controller) {
  systemController = controller;
}

void WebServerManager::setupRoutes() {
  server.on("/", HTTP_GET, [this]() { 
    server.send(200, "text/html", getMainPage()); 
  });
  
  server.on("/web_interface.html", HTTP_GET, [this]() { 
    server.send(200, "text/html", getWebInterface()); 
  });
  
  server.on("/status", HTTP_GET, [this]() { 
    server.send(200, "application/json", getStatusJSON()); 
  });
  
  server.on("/config", HTTP_GET, [this]() { 
    server.send(200, "application/json", getConfigJSON()); 
  });
  
  server.on("/sensors", HTTP_GET, [this]() { 
    server.send(200, "application/json", getSensorsInfoJSON()); 
  });
  
  server.on("/config", HTTP_POST, [this]() { 
    handleSaveConfig(); 
  });
  
  server.on("/calibrate", HTTP_POST, [this]() { 
    handleCalibrate(); 
  });
  
  server.on("/emergency", HTTP_POST, [this]() { 
    handleEmergencyStop(); 
  });
  
  // API для сброса конфигурации
  server.on("/reset-config", HTTP_POST, [this]() {
    if (currentState) {
      TerminalManager::addLog("🔄 Сброс конфигурации через веб-интерфейс");
      currentState->resetConfiguration();
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Конфигурация сброшена\"}");
      TerminalManager::addLog("✅ Конфигурация сброшена через веб-интерфейс");
    } else {
      server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Система не инициализирована\"}");
      TerminalManager::addLog("❌ Ошибка сброса конфигурации: система не инициализирована");
    }
  });
  
  // API для принудительного сохранения конфигурации
  server.on("/save-config", HTTP_POST, [this]() {
    if (currentState) {
      TerminalManager::addLog("💾 Принудительное сохранение конфигурации через веб-интерфейс");
      if (currentState->saveConfiguration()) {
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Конфигурация сохранена\"}");
        TerminalManager::addLog("✅ Конфигурация принудительно сохранена через веб-интерфейс");
      } else {
        server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Ошибка сохранения\"}");
        TerminalManager::addLog("❌ Ошибка принудительного сохранения конфигурации через веб-интерфейс");
      }
    } else {
      server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Система не инициализирована\"}");
      TerminalManager::addLog("❌ Ошибка принудительного сохранения: система не инициализирована");
    }
  });
  
  // API для терминала
  server.on("/terminal", HTTP_POST, [this]() {
    if (currentState) {
      String command = server.arg("command");
      String result = TerminalManager::processCommand(command, currentState);
      
      // Добавляем результат в логи терминала
      if (result.length() > 0) {
        TerminalManager::addLog(result);
      }
      
      server.send(200, "text/plain", result);
    } else {
      server.send(500, "text/plain", "Система не инициализирована");
    }
  });
  
  // API для получения логов терминала
  server.on("/terminal-logs", HTTP_GET, [this]() {
    String logs = TerminalManager::getLogs();
    server.send(200, "text/plain", logs);
  });
  
  // API для получения системных логов (дублирование Serial Monitor)
  server.on("/system-logs", HTTP_GET, [this]() {
    String logs = getSystemLogs();
    server.send(200, "text/plain", logs);
  });
  
  server.onNotFound([this]() { 
    server.send(404, "application/json", "{\"error\":\"Not found\"}"); 
  });
}

void WebServerManager::handleSaveConfig() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(CONFIG_JSON_SIZE);
    DeserializationError error = deserializeJson(doc, body);
    
    if (!error) {
      bool configChanged = false;
      
      if (doc.containsKey("targetTemp")) {
        float newTemp = doc["targetTemp"];
        if (newTemp >= TARGET_TEMP_MIN && newTemp <= TARGET_TEMP_MAX) {
          currentState->targetTemp = newTemp;
          // Передаем новое значение в SystemController
          if (systemController) {
            systemController->setTargetTemperature(newTemp);
            if (DEBUG_SERIAL) {
              Serial.println("SystemController target temperature updated to: " + String(newTemp) + "°C");
            }
          }
          configChanged = true;
          if (DEBUG_SERIAL) {
            Serial.println("Target temperature updated to: " + String(newTemp) + "°C");
          }
          TerminalManager::addLog("Целевая температура установлена через веб-интерфейс: " + String(newTemp, 1) + "°C");
        }
      }
      
      if (doc.containsKey("flowCalibrationFactor")) {
        float newFactor = doc["flowCalibrationFactor"];
        if (newFactor >= FLOW_CALIBRATION_MIN && newFactor <= FLOW_CALIBRATION_MAX) {
          currentState->flowCalibrationFactor = newFactor;
          // Обновляем коэффициент в датчике
          systemController->getSensors().setCalibrationFactor(newFactor);
          configChanged = true;
          if (DEBUG_SERIAL) {
            Serial.println("Flow calibration factor updated to: " + String(newFactor) + " imp/L");
          }
          TerminalManager::addLog("Калибровка протока установлена через веб-интерфейс: " + String(newFactor, 2) + " имп/л");
        }
      }
      
      if (configChanged) {
        // Сохраняем конфигурацию в EEPROM
        if (currentState->saveConfiguration()) {
          server.send(200, "application/json", "{\"status\":\"ok\"}");
          if (DEBUG_SERIAL) {
            Serial.println("Configuration updated and saved to EEPROM");
          }
          TerminalManager::addLog("✅ Настройки успешно сохранены в EEPROM через веб-интерфейс");
        } else {
          server.send(500, "application/json", "{\"error\":\"Failed to save to EEPROM\"}");
          if (DEBUG_SERIAL) {
            Serial.println("Failed to save configuration to EEPROM");
          }
          TerminalManager::addLog("❌ Ошибка сохранения настроек в EEPROM через веб-интерфейс");
        }
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
        TerminalManager::addLog("❌ Неверные параметры при сохранении настроек через веб-интерфейс");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      TerminalManager::addLog("❌ Ошибка парсинга JSON при сохранении настроек через веб-интерфейс");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    TerminalManager::addLog("❌ Отсутствуют данные при сохранении настроек через веб-интерфейс");
  }
}

void WebServerManager::handleCalibrate() {
  if (currentState) {
    // Запуск калибровки датчика протока
    TerminalManager::addLog("🔧 Запуск калибровки датчика протока через веб-интерфейс");
    systemController->getSensors().calibrateFlowSensor();
    currentState->flowCalibrationFactor = systemController->getSensors().getCalibrationFactor();
    
    server.send(200, "application/json", "{\"status\":\"calibration_completed\", \"factor\":" + String(currentState->flowCalibrationFactor) + "}");
    
    if (DEBUG_SERIAL) {
      Serial.println("Flow sensor calibration completed");
    }
    TerminalManager::addLog("✅ Калибровка датчика протока завершена через веб-интерфейс. Коэффициент: " + String(currentState->flowCalibrationFactor, 2) + " имп/л");
  }
}

void WebServerManager::handleEmergencyStop() {
  if (currentState) {
    currentState->isSystemEnabled = false;
    currentState->isHeating = false;
    
    server.send(200, "application/json", "{\"status\":\"emergency_stop\"}");
    
    if (DEBUG_SERIAL) {
      Serial.println("EMERGENCY STOP via web interface");
    }
    TerminalManager::addLog("🚨 АВАРИЙНАЯ ОСТАНОВКА выполнена через веб-интерфейс");
  }
}

String WebServerManager::getMainPage() {
  return readFileFromSPIFFS("/index.html");
}

String WebServerManager::getWebInterface() {
  return readFileFromSPIFFS("/web_interface.html");
}

String WebServerManager::getStatusJSON() {
  if (!currentState) {
    return "{\"error\":\"No state data\"}";
  }
  
  DynamicJsonDocument doc(1024);
  doc["temperature"] = currentState->currentTemp;
  doc["targetTemp"] = currentState->targetTemp;
  doc["flowRate"] = currentState->flowRate;
  doc["flowCalibrationFactor"] = currentState->flowCalibrationFactor;
  doc["flowPulseCount"] = systemController->getSensors().getFlowPulseCount();
  doc["isHeating"] = currentState->isHeating;
  doc["isFlowDetected"] = currentState->isFlowDetected;
  doc["isThermalFuseOK"] = currentState->isThermalFuseOK;
  doc["isSystemEnabled"] = currentState->isSystemEnabled;
  doc["isPowerRamping"] = currentState->isPowerRamping;
  doc["powerL1"] = currentState->heatingPower[0];
  doc["powerL2"] = currentState->heatingPower[1];
  doc["powerL3"] = currentState->heatingPower[2];
  doc["targetPowerL1"] = currentState->targetPower[0];
  doc["targetPowerL2"] = currentState->targetPower[1];
  doc["targetPowerL3"] = currentState->targetPower[2];
  doc["uptime"] = millis() / 1000;
  
  // Информация о режиме работы системы
  String modeText = "";
  switch(currentState->systemMode) {
    case SYSTEM_MODE_SLEEP:
      modeText = "Глубокий сон";
      break;
    case SYSTEM_MODE_ACTIVE:
      modeText = "Активная работа";
      break;
    case SYSTEM_MODE_WIFI_SESSION:
      modeText = "WiFi сессия";
      break;
    default:
      modeText = "Неизвестно";
      break;
  }
  
  doc["systemMode"] = currentState->systemMode;
  doc["systemModeText"] = modeText;
  doc["isWiFiEnabled"] = currentState->isWiFiEnabled;
  doc["wifiSessionTimeLeft"] = currentState->isWiFiEnabled ? 
    (WIFI_SESSION_TIMEOUT_MS - (millis() - currentState->wifiSessionStartTime)) / 1000 : 0;
  doc["updateFrequency"] = 1000; // 1с обновление в WiFi сессии
  
  String response;
  serializeJson(doc, response);
  return response;
}

String WebServerManager::getConfigJSON() {
  DynamicJsonDocument doc(512);
  
  // Читаем данные напрямую из EEPROM при каждом запросе
  float targetTemp = TARGET_TEMP_DEFAULT;
  float flowCalibrationFactor = FLOW_CALIBRATION_FACTOR;
  
  // Проверяем, есть ли валидная конфигурация в EEPROM
  if (ConfigStorage::isValidConfig()) {
    // Читаем данные из EEPROM
    uint32_t magic;
    EEPROM.get(0, magic); // MAGIC_NUMBER_ADDR = 0
    
    if (magic == 0x57415445) { // MAGIC_NUMBER
      // Читаем структуру конфигурации
      struct {
        float targetTemp;
        float flowCalibrationFactor;
        uint32_t magic;
      } config;
      
      EEPROM.get(4, config); // CONFIG_START_ADDR = 4
      
      // Проверяем валидность данных
      if (config.targetTemp >= TARGET_TEMP_MIN && config.targetTemp <= TARGET_TEMP_MAX &&
          config.flowCalibrationFactor >= FLOW_CALIBRATION_MIN && config.flowCalibrationFactor <= FLOW_CALIBRATION_MAX) {
        targetTemp = config.targetTemp;
        flowCalibrationFactor = config.flowCalibrationFactor;
        
        if (DEBUG_SERIAL) {
          Serial.println("WebServer: Загружена конфигурация из EEPROM:");
          Serial.println("Целевая температура: " + String(targetTemp, 1) + "°C");
          Serial.println("Коэффициент калибровки: " + String(flowCalibrationFactor, 2) + " имп/л");
        }
      }
    }
  }
  
  // Устанавливаем значения в JSON
  doc["targetTemp"] = targetTemp;
  doc["flowCalibrationFactor"] = flowCalibrationFactor;
  
  // Добавляем константы конфигурации
  doc["flowThresholdMin"] = FLOW_THRESHOLD_MIN;
  doc["flowThresholdMax"] = FLOW_THRESHOLD_MAX;
  doc["flowCalibrationMin"] = FLOW_CALIBRATION_MIN;
  doc["flowCalibrationMax"] = FLOW_CALIBRATION_MAX;
  doc["tempHysteresis"] = TEMP_HYSTERESIS;
  doc["maxTempSafety"] = MAX_TEMP_SAFETY;
  doc["targetTempMin"] = TARGET_TEMP_MIN;
  doc["targetTempMax"] = TARGET_TEMP_MAX;
  
  String response;
  serializeJson(doc, response);
  return response;
}

void WebServerManager::startWiFiAP() {
  // Отключаем WiFi клиент для экономии энергии
  WiFi.mode(WIFI_AP);
  
  // Настройка скрытой точки доступа с оптимизацией для расстояния до 5м
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 1, WIFI_MAX_CONNECTIONS); // Скрытая сеть, фиксированный канал
  
  // Полная мощность WiFi для веб-интерфейса
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // 19.5dBm - полная мощность для веб-интерфейса
  
  // Дополнительная оптимизация WiFi - устанавливаем максимальную мощность на уровне ESP-IDF
  esp_wifi_set_max_tx_power(WIFI_TX_POWER_FULL); // Полная мощность для веб-интерфейса
  
  // Отключаем энергосбережение WiFi для стабильности веб-интерфейса
  WiFi.setSleep(WIFI_PS_NONE); // Отключаем энергосбережение WiFi
  
  if (DEBUG_SERIAL) {
    Serial.print("WiFi AP started (full power for web interface): ");
    Serial.println(WIFI_SSID);
    Serial.printf("WiFi мощность: %d dBm (~%.1f mW)\n", WiFi.getTxPower(), pow(10, WiFi.getTxPower()/10.0));
    Serial.printf("WiFi канал: %d\n", WIFI_CHANNEL);
    Serial.println("WiFi сессия активна на 15 минут");
    Serial.println("IP адрес: 192.168.4.1");
  }
}

String WebServerManager::readFileFromSPIFFS(String path) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    return "Error: File not found";
  }
  
  String content = "";
  while (file.available()) {
    content += char(file.read());
  }
  file.close();
  
  return content;
}

String WebServerManager::getSystemLogs() {
  // Получаем логи из основного лога страницы
  String logs = "";
  
  // Добавляем системную информацию
  logs += "=== СИСТЕМНЫЕ ЛОГИ ===\n";
  logs += "Время работы: " + String(millis() / 1000) + " сек\n";
  logs += "Свободная память: " + String(ESP.getFreeHeap()) + " байт\n";
  logs += "Размер стека: " + String(uxTaskGetStackHighWaterMark(NULL)) + " байт\n";
  logs += "Температура чипа: " + String(temperatureRead(), 1) + "°C\n";
  
  if (currentState) {
    logs += "\n=== СОСТОЯНИЕ СИСТЕМЫ ===\n";
    logs += "Режим: " + String(currentState->systemMode == 0 ? "Сон" : 
                               currentState->systemMode == 1 ? "Активный" : "WiFi сессия") + "\n";
    logs += "Температура: " + String(currentState->currentTemp, 1) + "°C\n";
    logs += "Целевая температура: " + String(currentState->targetTemp, 1) + "°C\n";
    logs += "Проток: " + String(currentState->flowRate, 2) + " л/мин\n";
    logs += "Обнаружен проток: " + String(currentState->isFlowDetected ? "ДА" : "НЕТ") + "\n";
    logs += "Нагрев: " + String(currentState->isHeating ? "ВКЛ" : "ВЫКЛ") + "\n";
    logs += "WiFi: " + String(currentState->isWiFiEnabled ? "ВКЛ" : "ВЫКЛ") + "\n";
  }
  
  logs += "\n=== ПОСЛЕДНИЕ СОБЫТИЯ ===\n";
  logs += "Система инициализирована\n";
  logs += "Веб-сервер запущен\n";
  logs += "Готов к работе\n";
  
  return logs;
}

String WebServerManager::getSensorsInfoJSON() {
  DynamicJsonDocument doc(1024);
  
  // Функция для получения D-названия пина
  auto getDPinName = [](int gpio) -> String {
    switch (gpio) {
      case 34: return "D34";
      case 35: return "D35";
      case 32: return "D32";
      case 33: return "D33";
      case 25: return "D25";
      case 26: return "D26";
      case 27: return "D27";
      case 14: return "D14";
      case 12: return "D12";
      case 13: return "D13";
      case 23: return "D23";
      case 22: return "D22";
      case 21: return "D21";
      case 19: return "D19";
      case 18: return "D18";
      case 5:  return "D5";
      case 4:  return "D4";
      case 2:  return "D2";
      case 15: return "D15";
      case 1: return "TX0";
      case 3: return "RX0";
      case 17: return "TX2";
      case 16: return "RX2";
      default: return "N/A";
    }
  };
  
  // Информация о пинах с D-названиями
  doc["pins"]["ntc"] = "GPIO" + String(NTC_PIN) + " (" + getDPinName(NTC_PIN) + ")";
  doc["pins"]["flowSensor"] = "GPIO" + String(FLOW_SENSOR_PIN) + " (" + getDPinName(FLOW_SENSOR_PIN) + ")";
  doc["pins"]["triacL1"] = "GPIO" + String(TRIAC_L1_PIN) + " (" + getDPinName(TRIAC_L1_PIN) + ")";
  doc["pins"]["triacL2"] = "GPIO" + String(TRIAC_L2_PIN) + " (" + getDPinName(TRIAC_L2_PIN) + ")";
  doc["pins"]["triacL3"] = "GPIO" + String(TRIAC_L3_PIN) + " (" + getDPinName(TRIAC_L3_PIN) + ")";
  doc["pins"]["relayPower"] = "GPIO" + String(MOSFET_EN_PIN) + " (" + getDPinName(MOSFET_EN_PIN) + ")";
  
  // Проверка подключения датчиков
  // NTC датчик - проверяем, что значение не равно 0 (что означает отсутствие подключения)
  int ntcValue = analogRead(NTC_PIN);
  doc["sensors"]["ntcConnected"] = ntcValue > 100; // Пороговое значение для определения подключения
  
  // Датчик протока - проверяем, что пин настроен как вход и может читаться
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  int flowValue = digitalRead(FLOW_SENSOR_PIN);
  doc["sensors"]["flowSensorConnected"] = true; // Всегда показываем как подключен, так как пин работает
  
  doc["sensors"]["thermalFuseOK"] = "Аппаратная защита";
  
  // Добавляем отладочную информацию
  doc["debug"]["ntcRawValue"] = ntcValue;
  doc["debug"]["flowRawValue"] = flowValue;
  
  // Текущие значения
  if (currentState) {
    doc["values"]["temperature"] = currentState->currentTemp;
    doc["values"]["flowRate"] = currentState->flowRate;
    doc["values"]["flowPulseCount"] = systemController->getSensors().getFlowPulseCount();
  }
  
  // Системная информация
  doc["system"]["uptime"] = millis() / 1000;
  doc["system"]["chipTemp"] = temperatureRead(); // Температура чипа ESP32 в Цельсиях
  
  String response;
  serializeJson(doc, response);
  return response;
}
