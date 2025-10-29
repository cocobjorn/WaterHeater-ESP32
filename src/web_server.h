#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "system_state.h"

// Предварительное объявление
class SystemController;

class WebServerManager {
public:
  void begin();
  void handleClient();
  void updateStatus(SystemState& state);
  void setSystemController(SystemController* controller);
  
  // Управление WiFi сессией
  void startWiFiSession();
  void stopWiFiSession();
  bool isWiFiSessionActive() const;
  unsigned long getSessionTimeLeft() const;
  
private:
  WebServer server;
  SystemState* currentState = nullptr;
  SystemController* systemController = nullptr;
  
  // Состояние WiFi сессии
  bool wifiSessionActive;
  unsigned long sessionStartTime;
  
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
  void setupRoutes();
  String readFileFromSPIFFS(String path);
  String getSystemLogs();
};

#endif
