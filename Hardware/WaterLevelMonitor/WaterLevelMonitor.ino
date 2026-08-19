// ============================================================
// WaterLevelMonitor.ino
// ============================================================
// PURPOSE:
// Main entry point for the ESP32-S3 Water Level Monitoring Firmware.
//
// SYSTEM OVERVIEW:
// This firmware provides a high-reliability industrial liquid level
// monitor utilizing:
// 1. ADS1115 16-bit ADC for precision 4-20mA pressure sensor reading.
// 2. Quectel EC200U LTE Cat-1 Modem for parallel Ubidots HTTP and HiveMQ MQTT telemetry.
// 3. Modbus RTU Slave over RS485 for PLC / SCADA integration.
// 4. Bluetooth Low Energy (BLE) for local wireless configuration & diagnostics.
// 5. Active Audible (Buzzer) & Visual (RGB LED) Alert System.
// 6. FreeRTOS Dual-Core Task Scheduling with Thread-Safe Mutex Locks.
//
// ARDUINO IDE MULTI-TAB ARCHITECTURE:
// Arduino IDE concatenates all .ino files in alphabetical order during
// compilation. This main file establishes the global library includes,
// forward function declarations, and system entry points (setup/loop).
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <QuectelEC200U.h>

// ============================================================
// FORWARD FUNCTION DECLARATIONS (PROTOTYPES)
// ============================================================
// Forward declarations ensure function visibility across all Arduino IDE tabs.

// EEPROM Persistence Functions (03_EEPROM.ino)
void saveAirBaseline(float airValue);
bool isCalibrated();
float loadAirBaseline();
void saveAlarmState(bool state);
bool loadAlarmState();

// ADS1115 & Sensor Helper Functions (04_ADS1115.ino)
bool isSensorConnected(float adcValue);

// Calibration Functions (05_Calibration.ino)
float calibrateAirOnStartup();
void updateAirBaseline();

// Level Calculation Functions (06_LevelCalculation.ino)
float getHeightFromADC(float adcValue);

// Alert & Indication Functions (07_Alert.ino)
void initBuzzerAndLED();
void playTone(int frequency, int duration, int volume = 128);
void setRGBColor(int color);

// EC200U Modem Driver Functions (09_EC200U.ino)
void flushModemRx();
bool sendAT(const String &cmd, const String &expect, unsigned long timeout);
bool waitForModemResponse(const String &expect, unsigned long timeout, String &collected);
bool waitForSendPrompt(unsigned long timeout);
void readUntilIdle(unsigned long maxTotal, unsigned long idleGap, String &collected);
void EC200U_powerOn();
bool waitForURC(const String &tag, unsigned long timeout, String &collected);
String readIMEI();
bool ensurePDP();
bool connectModem();
void closeSocket();

// MQTT Functions (10_MQTT.ino)
bool connectMQTT();
bool publishMQTT(const char* payload);

// Ubidots HTTP Functions (11_Ubidots.ino)
bool sendToUbidots(float liquidLevel, float pressure, int sensorStatus);

// Modbus RTU Functions (12_Modbus.ino)
uint16_t ModRTU_CRC(uint8_t *buf, int len);
void enableTX();
void enableRX();
void sendResponse(uint8_t *frame, int len);

// FreeRTOS Task Entry Points (13_SensorTask.ino - 18_AlertTask.ino)
void SensorTask(void *pvParameters);
void UploadTask(void *pvParameters);
void MQTTTask(void *pvParameters);
void BLETask(void *pvParameters);
void ModbusTask(void *pvParameters);
void AlertTask(void *pvParameters);

// System Setup Routine (19_SystemSetup.ino)
void initializeSystem();

// ============================================================
// MAIN ARDUINO SETUP ENTRY POINT
// ============================================================
void setup()
{
    // Execute complete system hardware, peripheral, and FreeRTOS task initialization.
    initializeSystem();
}

// ============================================================
// MAIN ARDUINO LOOP ENTRY POINT
// ============================================================
void loop()
{
    // Application logic is managed autonomously by dedicated FreeRTOS tasks.
    // Delay loop to prevent background watchdog starvation.
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
