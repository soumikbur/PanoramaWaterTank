// ============================================================
// 19_SystemSetup.ino
// ============================================================
// PURPOSE:
// Complete Hardware and FreeRTOS System Initialization Tab.
//
// RESPONSIBILITIES:
// 1. Initialize USB Serial CDC communication at 115200 baud.
// 2. Control EC200U power ON sequence and execute cellular modem initialization.
// 3. Configure ESP32 analog resolution (12-bit).
// 4. Initialize Wire I2C interface (SDA pin 14, SCL pin 15) and ADS1115 ADC module.
// 5. Create FreeRTOS dataMutex and modemMutex synchronization objects.
// 6. Spawn all 6 FreeRTOS application tasks pinned to their specified cores.
//
// INITIALIZATION SEQUENCE (AUTHORITATIVE):
// Serial → EC200U Power ON → Cellular Modem Connect → ADC Resolution →
// Wire I2C → ADS1115 Detect → Mutex Allocation → FreeRTOS Task Creation.
//
// USED BY:
// - WaterLevelMonitor.ino (setup() invokes initializeSystem())
// ============================================================

// ============================================================
// SYSTEM INITIALIZATION ROUTINE
// ============================================================
void initializeSystem()
{
    // -------------------------------------------------
    // 1. Serial Port Initialization
    // -------------------------------------------------
    Serial.begin(115200);

#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);
#endif

    delay(3000);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" ESP32-S3 Water Level Monitor");
    Serial.println(" BLE + EC200U + Modbus + Ubidots");
    Serial.println("========================================");

    // -------------------------------------------------
    // 2. Power ON EC200U Modem & Initialize Cellular Network
    // -------------------------------------------------
    EC200U_powerOn();

    Serial.println("Initializing EC200U & Cellular PDP Context...");
    connectModem();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // -------------------------------------------------
    // 3. Configure ADC Resolution
    // -------------------------------------------------
    analogReadResolution(12);

    // -------------------------------------------------
    // 4. Initialize Wire I2C Bus
    // -------------------------------------------------
    Wire.begin(14, 15);

    Serial.println("I2C Initialized");

    // -------------------------------------------------
    // 5. Initialize ADS1115 16-Bit ADC
    // -------------------------------------------------
    if (!ads.begin(ADS1115_ADDR))
    {
        Serial.println("ERROR: ADS1115 NOT FOUND!");

        while (1)
        {
            delay(1000);
        }
    }

    Serial.println("ADS1115 Detected");

    // Set gain to GAIN_ONE (+/-4.096V range, 0.125mV/bit)
    ads.setGain(GAIN_ONE);

    Serial.println("Gain: GAIN_ONE (+/-4.096V)");

    // -------------------------------------------------
    // 6. Create FreeRTOS Mutex Synchronization Objects
    // -------------------------------------------------
    dataMutex = xSemaphoreCreateMutex();
    modemMutex = xSemaphoreCreateMutex();

    if (dataMutex == NULL)
    {
        Serial.println("ERROR: Failed to create data mutex!");

        while (1)
        {
            delay(1000);
        }
    }

    if (modemMutex == NULL)
    {
        Serial.println("ERROR: Failed to create modem mutex!");

        while (1)
        {
            delay(1000);
        }
    }

    // -------------------------------------------------
    // 7. Create Dual-Core FreeRTOS Application Tasks
    // -------------------------------------------------

    // Sensor Task (Core 0, Priority 2, Stack 8192)
    xTaskCreatePinnedToCore(
        SensorTask,
        "SensorTask",
        8192,
        NULL,
        2,
        &SensorTaskHandle,
        0);

    // Ubidots Upload Task (Core 1, Priority 1, Stack 8192)
    xTaskCreatePinnedToCore(
        UploadTask,
        "UploadTask",
        8192,
        NULL,
        1,
        &UploadTaskHandle,
        1);

    // BLE Server & Telemetry Task (Core 1, Priority 1, Stack 4096)
    xTaskCreatePinnedToCore(
        BLETask,
        "BLETask",
        4096,
        NULL,
        1,
        &BLETaskHandle,
        1);

    // Alarm & Visual Alert Task (Core 1, Priority 1, Stack 4096)
    xTaskCreatePinnedToCore(
        AlertTask,
        "AlertTask",
        4096,
        NULL,
        1,
        &AlertTaskHandle,
        1);

    // Modbus RTU Slave Task (Core 1, Priority 1, Stack 4096)
    xTaskCreatePinnedToCore(
        ModbusTask,
        "ModbusTask",
        4096,
        NULL,
        1,
        &ModbusTaskHandle,
        1);

    // HiveMQ Cellular MQTT Task (Core 1, Priority 1, Stack 8192)
    xTaskCreatePinnedToCore(
        MQTTTask,
        "MQTTTask",
        8192,
        NULL,
        1,
        &MQTTTaskHandle,
        1);

    Serial.println();
    Serial.println("All tasks created successfully");
    Serial.println("System initialization complete");
    Serial.println("========================================");
}
