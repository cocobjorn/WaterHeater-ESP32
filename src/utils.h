#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "config.h"

class Utils {
public:
  // Алгоритм Брезенхэма для плавного изменения мощности
  static void bresenhamPowerControl(uint8_t currentPower[3], uint8_t targetPower[3], uint8_t step = 1);
  
  // Фильтрация данных
  static float movingAverage(float* buffer, uint8_t size, float newValue);
  static float medianFilter(float* buffer, uint8_t size);
  
  // Математические функции
  static float mapFloat(float value, float fromMin, float fromMax, float toMin, float toMax);
  static float constrainFloat(float value, float minVal, float maxVal);
  
  // Время и задержки
  static bool isTimeout(unsigned long startTime, unsigned long timeout);
  static unsigned long getUptime();
  
  // Безопасность
  static bool isValidTemperature(float temp);
  static bool isValidFlowRate(float flow);
  static bool isValidPower(uint8_t power);
};

#endif
