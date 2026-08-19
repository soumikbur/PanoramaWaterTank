// ============================================================
// 18_AlertTask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated Alarm & Visual Indication Task (Core 1, Priority 1, Stack 4096).
//
// RESPONSIBILITIES:
// 1. Initialize buzzer PWM timer and RGB LED pins (initBuzzerAndLED()).
// 2. Monitor shared liquid level and alarm enable state under dataMutex protection.
// 3. Trigger Critical Level Alert when water level < 5 cm:
//    - Print critical serial alert message.
//    - Play 3000 Hz tone pattern for 100 ms (repeated 3 times).
//    - Flash RGB LED in red mode (setRGBColor(4)).
// 4. Trigger Warning Level Alert when water level < 10 cm:
//    - Print warning serial alert message.
//    - Play 2000 Hz tone pattern for 200 ms.
// 5. Silence buzzer and restore normal state when level >= 10 cm or alarm is disabled.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 1
// - Task Priority: 1
// - Stack Allocation: 4096 bytes
// ============================================================

void AlertTask(void *pvParameters) {
  Serial.println(" Alert Task running on Core 1");

  // Initialize hardware buzzer PWM and RGB LED GPIO
  initBuzzerAndLED();

  int lastAlertState = -1;
  static bool isOverflowing = false;

  while (1) {
    if (dataReady) {
      float level;
      int status;

      // Safely read shared telemetry from memory under dataMutex lock
      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        level = sharedLiquidLevel;
        status = sharedSensorStatus;
        xSemaphoreGive(dataMutex);
      }

      // Check sensor validity first
      if (status == 1 && level >= 0) {
        // Hysteresis logic for Overflow condition (50.0 cm threshold, 49.5 cm clear threshold)
        if (!isOverflowing && level >= TANK_HEIGHT_CM) {
          isOverflowing = true;
        } else if (isOverflowing && level <= (TANK_HEIGHT_CM - OVERFLOW_HYSTERESIS_CM)) {
          isOverflowing = false;
        }

        // 1. HIGH WATER / OVERFLOW ALARM (>= TANK_HEIGHT_CM, highest priority)
        if (isOverflowing) {
          if (lastAlertState != 3) {
            Serial.println(" [ALARM] OVERFLOW: Water height " + String(level, 2) + " cm exceeds max height (" + String(TANK_HEIGHT_CM, 2) + " cm)!");
            lastAlertState = 3;
          }
          if (alarmEnabled) {
            for (int i = 0; i < 3; i++) {
              playTone(3000, 100, 200);
              setRGBColor(4);
              delay(100);
            }
          } else {
            ledcWrite(BUZZER, 0);
            setRGBColor(0);
          }
        }
        // 2. CRITICAL LEVEL ALERT (< 5.0 cm)
        else if (level < CRITICAL_LEVEL_THRESHOLD && level > 0) {
          if (lastAlertState != 2) {
            Serial.println(" CRITICAL: Water level below " + String(CRITICAL_LEVEL_THRESHOLD) + " cm!");
            lastAlertState = 2;
          }
          if (alarmEnabled) {
            for (int i = 0; i < 3; i++) {
              playTone(3000, 100, 200);
              setRGBColor(4);
              delay(100);
            }
          } else {
            ledcWrite(BUZZER, 0);
            setRGBColor(0);
          }
        } 
        // 3. WARNING LEVEL ALERT (< 10.0 cm)
        else if (level < LOW_LEVEL_THRESHOLD && level > 0) {
          if (lastAlertState != 1) {
            Serial.println(" WARNING: Water level below " + String(LOW_LEVEL_THRESHOLD) + " cm!");
            lastAlertState = 1;
          }
          if (alarmEnabled) {
            playTone(2000, 200, 150);
            setRGBColor(1);
            delay(300);
          } else {
            ledcWrite(BUZZER, 0);
            setRGBColor(0);
          }
        } 
        // 4. NORMAL WATER LEVEL (10.0 cm ... < 50.0 cm)
        else {
          if (lastAlertState != 0) {
            Serial.println(" Water level normal: " + String(level, 2) + " cm");
            lastAlertState = 0;
          }
          ledcWrite(BUZZER, 0);
          setRGBColor(0);
        }
      } else {
        // Silence buzzer if sensor disconnected/invalid
        ledcWrite(BUZZER, 0);
        setRGBColor(0);
        lastAlertState = -1;
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
