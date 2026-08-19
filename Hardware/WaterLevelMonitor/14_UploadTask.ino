// ============================================================
// 14_UploadTask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated Ubidots Cloud HTTP Upload Task (Core 1, Priority 1, Stack 8192).
//
// RESPONSIBILITIES:
// 1. Wait until cellular modem initialization complete (modemReady == true).
// 2. Schedule cloud upload cycles every updateFrequency milliseconds (180 seconds = 3 minutes).
// 3. Take consistent sensor-data snapshot from shared memory under dataMutex protection.
// 4. Acquire modemMutex to gain exclusive lock over EC200U UART bus.
// 5. Execute HTTP POST upload to Ubidots REST API (sendToUbidots()).
// 6. Release modemMutex after upload completes or fails.
// 7. Schedule controlled 5-second retry delay upon upload failure.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 1
// - Task Priority: 1
// - Stack Allocation: 8192 bytes
// ============================================================

void UploadTask(void *pvParameters)
{
    Serial.println("Upload Task running on Core 1");

    // -------------------------------------------------
    // 1. Wait for EC200U modem initialization
    // -------------------------------------------------
    Serial.println("[UploadTask] Waiting for modem initialization...");

    while (!modemReady)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    Serial.println("[UploadTask] Modem is ready.");

    // -------------------------------------------------
    // 2. Initialize upload timer
    // -------------------------------------------------
    lastUpload = millis();

    // -------------------------------------------------
    // 3. Main upload loop
    // -------------------------------------------------
    while (true)
    {
        if (!modemReady)
        {
            Serial.println("[UploadTask] Modem unavailable.");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        const uint32_t now = millis();

        if ((uint32_t)(now - lastUpload) < updateFrequency)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        Serial.println();
        Serial.println("========================================");
        Serial.println("[UploadTask] Upload interval reached");

        // -------------------------------------------------
        // 4. Take consistent sensor-data snapshot
        // -------------------------------------------------
        bool ready = false;
        float level = 0.0f;
        float pressure = 0.0f;
        int status = 0;

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            ready = dataReady;

            if (ready)
            {
                level = sharedLiquidLevel;
                pressure = sharedPressure;
                status = sharedSensorStatus;
            }

            xSemaphoreGive(dataMutex);
        }
        else
        {
            Serial.println("[UploadTask] ERROR: Data mutex timeout.");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        Serial.print("[UploadTask] dataReady = ");
        Serial.println(ready ? "TRUE" : "FALSE");

        if (!ready)
        {
            Serial.println("[UploadTask] Waiting for sensor data...");
            Serial.println("========================================");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        Serial.println("----------------------------------------");
        Serial.println("Sensor Data");
        Serial.print("Level    : ");
        Serial.println(level, 2);
        Serial.print("Pressure : ");
        Serial.println(pressure, 2);
        Serial.print("Status   : ");
        Serial.println(status);
        Serial.println("----------------------------------------");

        // -------------------------------------------------
        // 5. Execute Ubidots HTTP Upload under modemMutex lock
        // -------------------------------------------------
        bool uploadSuccess = false;

        Serial.println("[UploadTask] Calling sendToUbidots()...");

        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(10000)) == pdTRUE)
        {
            if (modemReady)
            {
                uploadSuccess = sendToUbidots(level, pressure, status);
            }
            else
            {
                Serial.println("[UploadTask] Modem became unavailable.");
            }

            xSemaphoreGive(modemMutex);
        }
        else
        {
            Serial.println("[UploadTask] ERROR: Modem mutex timeout.");
        }

        // -------------------------------------------------
        // 6. Process upload result
        // -------------------------------------------------
        if (uploadSuccess)
        {
            lastUpload = millis();

            Serial.println("[UploadTask] Ubidots upload SUCCESS");
            Serial.print("[UploadTask] Next upload in ");
            Serial.print(updateFrequency / 1000UL);
            Serial.println(" seconds");
        }
        else
        {
            Serial.println("[UploadTask] Ubidots upload FAILED");

            // Controlled 5-second retry delay upon failure.
            const uint32_t RETRY_DELAY_MS = 5000UL;
            lastUpload = millis() - updateFrequency + RETRY_DELAY_MS;

            Serial.println("[UploadTask] Retry scheduled in 5 seconds.");
        }

        Serial.println("========================================");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
