#ifndef CONFIG_H
#define CONFIG_H

// WiFi настройки
#define WIFI_SSID "WaterHeater"
#define WIFI_PASSWORD "12345678"
#define WIFI_AP_MODE true
#define WIFI_TX_POWER_FULL WIFI_POWER_19_5dBm  // Полная мощность для веб-интерфейса
#define WIFI_TX_POWER_LOW WIFI_POWER_5dBm      // Минимальная мощность для пробуждения
#define WIFI_CHANNEL 1       // Фиксированный канал для стабильности
#define WIFI_MAX_CONNECTIONS 4  // Максимум 4 подключения
#define WIFI_SESSION_TIMEOUT_MS 900000  // 15 минут работы WiFi

// Кнопка управления
#define BOOT_BUTTON_PIN 0    // GPIO0 - кнопка BOOT
#define BOOT_BUTTON_DEBOUNCE_MS 50
#define BOOT_DOUBLE_CLICK_TIMEOUT_MS 500  // Таймаут для двойного нажатия

// Режимы работы
#define SYSTEM_MODE_SLEEP 0
#define SYSTEM_MODE_ACTIVE 1
#define SYSTEM_MODE_WIFI_SESSION 2

// Пины ESP32 с платой расширения HW-777
#define NTC_PIN 34
#define FLOW_SENSOR_PIN 35
#define TRIAC_L1_PIN 21
#define TRIAC_L2_PIN 19
#define TRIAC_L3_PIN 18
// Пин детектора нулевого перехода сети (для фазового управления MOC3023)
#define ZERO_CROSS_PIN 15

// Управление модулем MOSFET (подача 5В на общий блок реле)
#define MOSFET_EN_PIN 4

// Светодиод индикации
#define STATUS_LED_PIN 2  // D2 - встроенный светодиод ESP32

// Настройки температуры
#define TARGET_TEMP_MIN 40.0
#define TARGET_TEMP_MAX 65.0
#define TARGET_TEMP_DEFAULT 50.0
#define MIN_TEMP 35.0
#define MAX_TEMP 70.0
#define TEMP_HYSTERESIS 2.0
#define MAX_TEMP_SAFETY 80.0
#define SAFETY_TIMEOUT 30000  // 30 секунд

// Настройки протока
#define FLOW_THRESHOLD_MIN 0.5  // л/мин
#define FLOW_THRESHOLD_MAX 1.0  // л/мин
#define FLOW_THRESHOLD 1.0      // л/мин
#define FLOW_HYSTERESIS 0.3     // л/мин - гистерезис для предотвращения ложных отключений
#define FLOW_TIMEOUT_MS 2000    // мс - время ожидания перед отключением при отсутствии протока
#define FLOW_CALIBRATION_FACTOR 7.5  // импульсов на литр (настраивается)
#define FLOW_CALIBRATION_MIN 1.0    // минимальный коэффициент калибровки
#define FLOW_CALIBRATION_MAX 1000.0 // максимальный коэффициент калибровки

// Настройки NTC
#define NTC_BETA 3950.0
#define NTC_NOMINAL_TEMP 25.0
#define NTC_NOMINAL_RESISTANCE 10000.0
#define NTC_SERIES_RESISTANCE 10000.0

// Мониторинг в Serial порт
#define SERIAL_MONITOR_INTERVAL 1000      // Интервал вывода данных в Serial (мс)

// Настройки нагрева
#define HEATING_POWER_STEPS 100  // шагов для алгоритма Bresenham
#define MIN_POWER_PERCENT 10
#define MAX_POWER_PERCENT 100
#define POWER_RAMP_TIME_MS 2000  // 2 секунды для плавного выхода на полную мощность
#define POWER_RAMP_STEP_MS 40    // шаг изменения мощности каждые 40мс для плавного разгона за 2 сек

// Настройки безопасности
#define THERMAL_FUSE_PIN 16  // Пин термопредохранителя
#define SAFETY_CHECK_INTERVAL 1000  // мс
#define MAX_HEATING_TIME 3000    // 3 секунды без протока для быстрого отключения
#define RELAY_SAFETY_DELAY 100  // Задержка включения реле для безопасности

// Дополнительные параметры безопасности
#define MAX_POWER_CHANGE_PER_CYCLE 5     // Максимальное изменение мощности за цикл (%) - уменьшено для плавного разгона
#define MAX_POWER_AT_LOW_FLOW 70          // Максимальная мощность при малом протоке (%) - увеличено
#define MAX_POWER_AT_HIGH_TEMP 50         // Максимальная мощность при высокой температуре (%) - увеличено
#define POWER_RAMP_TIMEOUT_MS 5000        // Таймаут разгона мощности (мс)
#define SYSTEM_WATCHDOG_TIMEOUT_MS 5000   // Таймаут watchdog системы (мс)
#define TEMP_RANGE_MIN -10.0              // Минимальная допустимая температура
#define TEMP_RANGE_MAX 82.0              // Максимальная допустимая температура
#define FLOW_RANGE_MAX 100.0              // Максимальный допустимый проток

// Веб-сервер
#define WEB_SERVER_PORT 80
#define CONFIG_JSON_SIZE 1024

// Отладка
#define DEBUG_SERIAL true
#define SENSOR_READ_INTERVAL 100  // мс

#endif
