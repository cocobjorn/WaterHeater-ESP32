#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "system_state.h"

class WebServerManager {
public:
  void begin();
  void handleClient();
  void updateStatus(SystemState& state);
  
private:
  WebServer server;
  SystemState* currentState = nullptr;
  
  // Обработчики веб-запросов
  void handleSaveConfig();
  void handleCalibrate();
  void handleEmergencyStop();
  
  // HTML страницы
  String getMainPage();
  String getWebInterface();
  String getStatusJSON();
  String getConfigJSON();
  String getSensorsInfoJSON();
  
  // Вспомогательные функции
  void startWiFiAP();
  String readFileFromSPIFFS(String path);
  String getSystemLogs();
};

#endif
