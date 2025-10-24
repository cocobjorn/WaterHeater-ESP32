#ifndef SAFETY_H
#define SAFETY_H

#include "config.h"
#include "system_state.h"

class SafetyManager {
public:
  void begin();
  void checkSafety(SystemState& state);
  void emergencyShutdown(class TriacControl* triacControl);
  void resetEmergencyMode();
  bool isThermalFuseOK();
  bool isSystemSafe();
  
private:
  unsigned long lastThermalCheck = 0;
  unsigned long lastFlowCheck = 0;
  unsigned long lastPowerCheck = 0;
  bool thermalFuseState = true;
  bool emergencyMode = false;
  unsigned long emergencyStartTime = 0;
  
  void checkBasicSafety(SystemState& state);
  void checkThermalFuse();
  void checkFlowTimeout(SystemState& state);
  void checkTemperatureLimits(SystemState& state);
  void checkPowerLimits(SystemState& state);
  void checkSystemIntegrity(SystemState& state);
  void logSafetyEvent(const char* message);
};

#endif
