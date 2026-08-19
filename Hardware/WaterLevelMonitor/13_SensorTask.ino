// ============================================================
// 13_SensorTask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated Sensor Acquisition Task (Core 0, Priority 2, Stack 8192).
//
// RESPONSIBILITIES:
// 1. Initialize Wire I2C interface (SDA pin 14, SCL pin 15) and ADS1115 16-bit ADC.
// 2. Perform initial air baseline calibration (calibrateAirOnStartup()).
// 3. Perform 10-sample averaging and Exponential Moving Average (EMA) filtering on raw ADC counts.
// 4. Periodically trigger automatic air baseline recalibration (every 5 minutes).
// 5. Evaluate physical 4-20mA sensor loop connection state against voltage disconnect threshold (0.25 V).
// 6. Calculate liquid level (cm) via lookup interpolation and pressure offset (ADC counts).
// 7. Safely update volatile shared variables (sharedLiquidLevel, sharedPressure, sharedSensorStatus) under dataMutex protection.
// 8. Support optional DEBUG_ADC 1 mode for raw ADC dump diagnostics.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 0
// - Task Priority: 2 (Highest application priority)
// - Stack Allocation: 8192 bytes
// ============================================================

void SensorTask(void *pvParameters) {
#if DEBUG_ADC
  // ===================== DEBUG_ADC MODE =====================
  // Minimal-latency raw ADC dump + computed voltage, ADS1115 channel 0 only.
  Serial.println(" Sensor Task running on Core 0 [DEBUG_ADC MODE]");

#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
#endif

  Wire.begin(14, 15);
  Wire.setTimeOut(50);

  Serial.println(" Scanning I2C bus...");
  int devicesFound = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("   Found device at 0x");
      Serial.println(addr, HEX);
      devicesFound++;
    }
  }
  if (devicesFound == 0) {
    Serial.println(" No I2C devices found at all - check wiring/power/pull-ups");
  } else {
    Serial.print(" ");
    Serial.print(devicesFound);
    Serial.println(" I2C device(s) found");
  }

  if (!ads.begin(ADS1115_ADDR)) {
    Serial.println(" ADS1115 not found");
    while (1)
      ;
  }

  ads.setGain(GAIN_ONE);
  Serial.println(" ADS1115 initialized (DEBUG_ADC, GAIN_ONE = +-4.096V)");

  while (1) {
    int16_t raw = ads.readADC_SingleEnded(0);
    float volts = ads.computeVolts(raw);

    Serial.print("Raw ADC: ");
    Serial.print(raw);
    Serial.print("   Voltage: ");
    Serial.print(volts, 4);
    Serial.println(" V");

    vTaskDelay(8 / portTICK_PERIOD_MS);
  }

#else
  // ===================== PRODUCTION MODE =====================
  Serial.println(" Sensor Task running on Core 0");

  Wire.begin(14, 15);

  if (!ads.begin(ADS1115_ADDR)) {
    Serial.println(" ADS1115 not found");
    while (1)
      ;
  }
  Serial.println(" ADS1115 initialized");

  alarmEnabled = loadAlarmState();
  Serial.print(" Alarm state loaded: ");
  Serial.println(alarmEnabled ? "ENABLED" : "DISABLED");

  A_air = calibrateAirOnStartup();

  Serial.println("\n========================================");
  Serial.println(" CALIBRATION SUMMARY");
  Serial.println("========================================");
  Serial.print("  A_air (0 cm): ");
  Serial.println(A_air, 2);
  Serial.print("  ADC per cm: ");
  Serial.println(ADC_PER_CM, 2);
  Serial.print("  Deadband: ");
  Serial.print(DEADBAND_THRESHOLD);
  Serial.println(" cm");
  Serial.print("  Alarm: ");
  Serial.println(alarmEnabled ? "ENABLED" : "DISABLED");
  Serial.println("========================================\n");

  filteredADC = A_air;
  lastRecalibration = millis();

  while (1) {
    // 1. Collect 10 raw ADC samples and compute mean average
    long sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
      sum += abs(ads.readADC_SingleEnded(0));
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    int16_t raw = sum / SAMPLES;

    // 2. Apply Exponential Moving Average (EMA) filter
    filteredADC = (alpha * raw) + ((1 - alpha) * filteredADC);

    // 3. Periodic automatic air baseline recalibration (every 5 minutes)
    if (millis() - lastRecalibration > RECALIBRATE_INTERVAL) {
      updateAirBaseline();
      lastRecalibration = millis();
    }

    // 4. Verify 4-20mA sensor connectivity
    bool sensorConnected = isSensorConnected(filteredADC);

    if (!sensorConnected) {
      badReadingCount++;
      if (badReadingCount >= DISCONNECTED_THRESHOLD) {
        sensorDisconnected = true;
        lastGoodReading = millis();
      }
    } else {
      badReadingCount = 0;
      sensorDisconnected = false;
      lastGoodReading = millis();
    }

    // 5. Compute liquid level and pressure offset
    float level = 0;
    float pressure = 0;
    int status = 0;

    if (sensorDisconnected) {
      level = 0;
      pressure = 0;
      status = 0;
    } else {
      level = getHeightFromADC(filteredADC);
      pressure = filteredADC - A_air;
      if (pressure < 0) pressure = 0;
      status = 1;
    }

    // 6. Thread-safe copy into global shared memory under dataMutex protection
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      sharedLiquidLevel = level;
      sharedPressure = pressure;
      sharedSensorStatus = status;
      dataReady = true;
      xSemaphoreGive(dataMutex);
    }

    // 7. Periodic diagnostic printout to Serial Monitor
    static int printCounter = 0;
    printCounter++;
    if (printCounter >= 5) {
      if (sensorDisconnected) {
        Serial.print(" ADC: ");
        Serial.print(filteredADC, 2);
        Serial.println(" | SENSOR DISCONNECTED");
      } else {
        float volts = ads.computeVolts(filteredADC);
        float zeroVolts = ads.computeVolts(A_air);
        float levelPercent = (level / TANK_HEIGHT_CM) * 100.0f;
        if (levelPercent > 100.0f) levelPercent = 100.0f;
        if (levelPercent < 0.0f) levelPercent = 0.0f;

        Serial.print(" ADC: ");
        Serial.print(filteredADC, 2);
        Serial.print(" | Voltage: ");
        Serial.print(volts, 4);
        Serial.print(" V | Zero: ");
        Serial.print(zeroVolts, 4);
        Serial.print(" V | Water Height: ");
        Serial.print(level, 2);
        Serial.print(" cm | Tank Height: ");
        Serial.print(TANK_HEIGHT_CM, 2);
        Serial.print(" cm | Level: ");
        Serial.print(levelPercent, 2);
        Serial.println("%");
      }
      printCounter = 0;
    }

    vTaskDelay(sensorReadInterval / portTICK_PERIOD_MS);
  }
#endif
}
