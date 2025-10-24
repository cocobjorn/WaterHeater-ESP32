#ifndef TRIAC_CONTROL_H
#define TRIAC_CONTROL_H

#include <stdint.h>
#include "config.h"
#include "system_state.h"

class TriacControl {
public:
  void begin();
  void setPower(uint8_t power[3]);
  void setPhasePower(uint8_t phase, uint8_t power);
  void normalStop();
  void emergencyStop();
  bool isPhaseActive(uint8_t phase);
  void smoothRampToPower(uint8_t targetPower[3], SystemState& state);
  void updateRamp(SystemState& state);
  void updateTriacs();
  
  // Функции для работы с детектором нуля
  bool readZeroCrossState();
  int getZeroCrossRawValue();
  void enableZeroCrossInterrupt();
  void disableZeroCrossInterrupt();
  
  // Функция для тестирования симисторов
  void testTriacs();
  
  // Функция для обработки флага прерывания в основном цикле
  void processZeroCrossFlag();
  
private:
  uint8_t currentPower[3] = {0, 0, 0};
  unsigned long lastUpdateTime = 0;
  
  // Переменные для детектора нуля
  static TriacControl* instance;
  volatile unsigned long lastZeroCrossMicros = 0;
  volatile bool zeroCrossEnabled = false;
  volatile bool zeroCrossFlag = false;
  static void zeroCrossInterrupt();
  
  // Псевдо фазо-управление для тестов с MOC3023 без детектора нуля
  unsigned long lastHalfStartUs = 0;
  static const uint32_t HALF_CYCLE_US = 10000; // 10мс при 50Гц
  bool firedThisHalf[3] = {false, false, false};
  
  // Для прежнего ШИМ API (не используется в фазовом тестовом режиме)
  static const uint16_t PWM_FREQUENCY = 1000;
  static const uint8_t PWM_RESOLUTION = 8;
  
  // Каналы ШИМ для ESP32
  static const uint8_t PWM_CHANNEL_L1 = 0;
  static const uint8_t PWM_CHANNEL_L2 = 1;
  static const uint8_t PWM_CHANNEL_L3 = 2;
  
  
  void rampPower(uint8_t targetPower[3]);
};

#endif
