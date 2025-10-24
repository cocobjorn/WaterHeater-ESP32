#include "utils.h"
#include <Arduino.h>
#include <HardwareSerial.h>

void Utils::bresenhamPowerControl(uint8_t currentPower[3], uint8_t targetPower[3], uint8_t step) {
  for (int i = 0; i < 3; i++) {
    int error = targetPower[i] - currentPower[i];
    int absError = abs(error);
    
    if (absError >= step) {
      if (error > 0) {
        currentPower[i] = min((uint8_t)(currentPower[i] + step), targetPower[i]);
      } else {
        currentPower[i] = max((uint8_t)(currentPower[i] - step), targetPower[i]);
      }
    } else {
      currentPower[i] = targetPower[i];
    }
  }
}

float Utils::movingAverage(float* buffer, uint8_t size, float newValue) {
  static uint8_t index = 0;
  
  buffer[index] = newValue;
  index = (index + 1) % size;
  
  float sum = 0;
  for (uint8_t i = 0; i < size; i++) {
    sum += buffer[i];
  }
  
  return sum / size;
}

float Utils::medianFilter(float* buffer, uint8_t size) {
  // Создаем копию для сортировки
  float sorted[10]; // Максимальный размер буфера
  if (size > 10) size = 10;
  
  for (uint8_t i = 0; i < size; i++) {
    sorted[i] = buffer[i];
  }
  
  // Простая сортировка пузырьком
  for (uint8_t i = 0; i < size - 1; i++) {
    for (uint8_t j = 0; j < size - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  // Возвращаем медиану
  if (size % 2 == 0) {
    return (sorted[size / 2 - 1] + sorted[size / 2]) / 2.0;
  } else {
    return sorted[size / 2];
  }
}

float Utils::mapFloat(float value, float fromMin, float fromMax, float toMin, float toMax) {
  return (value - fromMin) * (toMax - toMin) / (fromMax - fromMin) + toMin;
}

float Utils::constrainFloat(float value, float minVal, float maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

bool Utils::isTimeout(unsigned long startTime, unsigned long timeout) {
  return (millis() - startTime) >= timeout;
}

unsigned long Utils::getUptime() {
  return millis() / 1000;
}

bool Utils::isValidTemperature(float temp) {
  return temp >= -10.0 && temp <= 100.0;
}

bool Utils::isValidFlowRate(float flow) {
  return flow >= 0.0 && flow <= 50.0;
}

bool Utils::isValidPower(uint8_t power) {
  return power <= 100;
}
