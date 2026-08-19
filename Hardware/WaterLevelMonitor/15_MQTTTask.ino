// ============================================================
// 15_MQTTTask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated HiveMQ Cellular MQTT Telemetry Task (Core 1, Priority 1, Stack 8192).
//
// RESPONSIBILITIES:
// 1. Wait until cellular network and PDP context are initialized (modemReady == true).
// 2. Perform initial MQTT connection to broker.hivemq.com:1883 (connectMQTT()).
// 3. Periodically sample shared sensor memory under dataMutex protection (every 5 seconds).
// 4. Construct telemetry JSON payload: {"waterlevel": X, "pressure": Y, "sensorstatus": Z}.
// 5. Acquire modemMutex lock before executing MQTT publish operations.
// 6. Transmit MQTT telemetry payload over cellular link (publishMQTT()).
// 7. Handle automatic reconnect logic upon publish failure.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 1
// - Task Priority: 1
// - Stack Allocation: 8192 bytes
// ============================================================

void MQTTTask(void *pvParameters)
{
    Serial.println("MQTT Task running on Core 1");

    // -------------------------------------------------
    // 1. Wait for EC200U cellular network & PDP context
    // -------------------------------------------------
    Serial.println("[MQTTTask] Waiting for modem initialization...");
    while (!modemReady)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    Serial.println("[MQTTTask] Modem initialization complete.");

    // -------------------------------------------------
    // 2. Connect MQTT initially under modemMutex lock
    // -------------------------------------------------
    if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(10000)) == pdTRUE)
    {
        connectMQTT();
        xSemaphoreGive(modemMutex);
    }

    // -------------------------------------------------
    // 3. Main MQTT Telemetry Loop
    // -------------------------------------------------
    while (true)
    {
        // Take atomic sensor snapshot from shared memory.
        float level = 0.0f;
        float pressure = 0.0f;
        int status = 0;

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            level = sharedLiquidLevel;
            pressure = sharedPressure;
            status = sharedSensorStatus;
            xSemaphoreGive(dataMutex);
        }

        // Format telemetry JSON payload.
        String payload = "{\"waterlevel\":" + String(level, 2) + 
                         ",\"pressure\":" + String(pressure, 2) + 
                         ",\"sensorstatus\":" + String(status) + "}";

        Serial.println();
        Serial.println("Publishing MQTT message...");
        Serial.print("Payload: ");
        Serial.println(payload);

        // Acquire modemMutex to publish over EC200U UART.
        if (xSemaphoreTake(modemMutex, pdMS_TO_TICKS(10000)) == pdTRUE)
        {
            if (!publishMQTT(payload.c_str()))
            {
                Serial.println("MQTT publish failed, attempting reconnect...");

                if (connectMQTT())
                {
                    Serial.println("MQTT reconnected, retrying publish...");
                    publishMQTT(payload.c_str());
                }
            }
            xSemaphoreGive(modemMutex);
        }
        else
        {
            Serial.println("[MQTTTask] ERROR: Modem mutex timeout.");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
