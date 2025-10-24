#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include "config.h"

class Sensors {
public:
  void begin();
  float readTemperature();
  float readFlowRate();
  void calibrateFlowSensor();
  void setCalibrationFactor(float factor);
  float getCalibrationFactor();
  unsigned long getFlowPulseCount();
  void resetFlowPulseCount();
  
private:
  static Sensors* instance;
  float calculateNTCtemperature(float resistance);
  volatile unsigned long flowPulseCount = 0;
  unsigned long lastFlowTime = 0;
  float flowCalibrationFactor = FLOW_CALIBRATION_FACTOR;
  
  // Фильтрация для стабильности показаний
  float tempFilter[5] = {0};
  uint8_t tempFilterIndex = 0;
  float flowFilter[3] = {0};
  uint8_t flowFilterIndex = 0;
  
  static void flowInterrupt();
};

#endif
