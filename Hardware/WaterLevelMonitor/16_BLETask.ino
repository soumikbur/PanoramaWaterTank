// ============================================================
// 16_BLETask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated Bluetooth Low Energy Task (Core 1, Priority 1, Stack 4096).
//
// RESPONSIBILITIES:
// 1. Initialize ESP32 NimBLE stack with device name "Water Level Sensor".
// 2. Instantiate BLE GATT server, custom service, RX write, and TX notify characteristics.
// 3. Register server callbacks (MyServerCallbacks) and characteristic write callbacks (MyCallbacks).
// 4. Start BLE advertising to allow smartphone app discovery.
// 5. Periodically transmit live telemetry notification strings every 2 seconds to connected clients.
// 6. Automatically resume advertising when a connected BLE client disconnects.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 1
// - Task Priority: 1
// - Stack Allocation: 4096 bytes
// ============================================================

void BLETask(void *pvParameters) {
  Serial.println(" BLE Task running on Core 1");

  // 1. Initialize BLE stack with device name
  BLEDevice::init("Water Level Sensor");

  // 2. Create BLE Server and register server lifecycle callbacks
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create Nordic UART BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Create TX notification characteristic
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);

  // 5. Create RX write characteristic and register command callbacks
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // 6. Start BLE Service and start advertising
  pService->start();
  pServer->getAdvertising()->start();

  Serial.println(" BLE advertising started");
  Serial.println("   Device Name: Water Level Sensor");

  // 7. Main BLE Telemetry Loop
  while (1) {
    if (deviceConnected) {
      static unsigned long lastBLESend = 0;

      // Transmit live telemetry string every 2000 ms to connected client
      if (millis() - lastBLESend > 2000) {
        if (dataReady) {
          float level;
          int status;

          if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            level = sharedLiquidLevel;
            status = sharedSensorStatus;
            xSemaphoreGive(dataMutex);
          }

          String bleData = "Level: " + String(level, 2) + " cm | Status: " + 
                           (status == 1 ? "NORMAL" : "DISCONNECTED") + 
                           " | Alarm: " + (alarmEnabled ? "ON" : "OFF");

          pTxCharacteristic->setValue(bleData.c_str());
          pTxCharacteristic->notify();
          lastBLESend = millis();
        }
      }
    }

    // Restart advertising if client disconnects
    if (!deviceConnected && oldDeviceConnected) {
      delay(500);
      pServer->getAdvertising()->start();
      Serial.println(" Started advertising again...");
      oldDeviceConnected = false;
    }

    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = true;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
