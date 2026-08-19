// ============================================================
// 17_ModbusTask.ino
// ============================================================
// PURPOSE:
// FreeRTOS Dedicated RS485 Modbus RTU Slave Server Task (Core 1, Priority 1, Stack 4096).
//
// RESPONSIBILITIES:
// 1. Initialize MAX485 transceiver DE/RE pin direction control (GPIO 4).
// 2. Start RS485Serial (Hardware Serial 2 at 9600 baud, 8N1, RX pin 16, TX pin 17).
// 3. Update Modbus holding registers 0, 1, 2 from shared memory:
//    - Register 0: Water Level (cm x 10)
//    - Register 1: Pressure (ADC x 10)
//    - Register 2: Sensor Status (0 = Disconnected, 1 = Normal)
// 4. Listen for RS485 frames matching Slave ID 1 and validate CRC-16.
// 5. Process Modbus Function 0x03 (Read Holding Registers).
// 6. Process Modbus Function 0x06 (Write Single Register 10 for air calibration update).
// 7. Transmit Modbus RTU response or error frames back to PLC master.
//
// FREERTOS PROPERTIES:
// - Core Assignment: Core 1
// - Task Priority: 1
// - Stack Allocation: 4096 bytes
// ============================================================

void ModbusTask(void *pvParameters) {
  Serial.println(" Modbus Task running on Core 1");

  pinMode(RS485_DE, OUTPUT);
  enableRX();
  RS485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.println(" Modbus RTU Slave initialized");
  Serial.print("   Slave ID: ");
  Serial.println(MODBUS_SLAVE_ID);
  Serial.println("   Register Map:");
  Serial.println("     Holding Register 0: Water Level (cm x 10)");
  Serial.println("     Holding Register 1: Pressure (ADC x 10)");
  Serial.println("     Holding Register 2: Sensor Status (0=Disconnected, 1=Normal)");

  static uint8_t buffer[64];
  int len = 0;

  while (1) {
    // 1. Read sensor telemetry from shared memory under dataMutex
    float level = 0;
    float pressure = 0;
    int status = 0;

    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      level = sharedLiquidLevel;
      pressure = sharedPressure;
      status = sharedSensorStatus;
      xSemaphoreGive(dataMutex);
    }

    // 2. Update Modbus holding register values
    holdingRegs[0] = (uint16_t)(level * 10);     // Level in cm x 10 (e.g., 25.4 -> 254)
    holdingRegs[1] = (uint16_t)(pressure * 10);  // Pressure in ADC x 10
    holdingRegs[2] = (uint16_t)status;           // Sensor status

    // 3. Read incoming RS485 Modbus requests
    while (RS485Serial.available()) {
      buffer[len++] = RS485Serial.read();
      delay(2);

      if (len >= sizeof(buffer)) {
        len = 0;
        break;
      }
    }

    if (len < 8) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }

    // 4. Validate Modbus Slave ID
    if (buffer[0] != MODBUS_SLAVE_ID) {
      len = 0;
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }

    // 5. Validate Modbus CRC-16 checksum
    uint16_t crc_calc = ModRTU_CRC(buffer, len - 2);
    uint16_t crc_recv = buffer[len - 2] | (buffer[len - 1] << 8);

    if (crc_calc != crc_recv) {
      len = 0;
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }

    uint8_t function = buffer[1];

    // -------------------------------------------------
    // FUNCTION 0x03: READ HOLDING REGISTERS
    // -------------------------------------------------
    if (function == 0x03) {
      uint16_t startAddr = (buffer[2] << 8) | buffer[3];
      uint16_t numRegs = (buffer[4] << 8) | buffer[5];

      // Validate address boundaries
      if (startAddr + numRegs > 20) {
        uint8_t errorResp[5];
        errorResp[0] = MODBUS_SLAVE_ID;
        errorResp[1] = 0x83;  // Function + 0x80 (error)
        errorResp[2] = 0x02;  // Illegal data address
        uint16_t errCrc = ModRTU_CRC(errorResp, 3);
        errorResp[3] = errCrc & 0xFF;
        errorResp[4] = errCrc >> 8;
        sendResponse(errorResp, 5);
        len = 0;
        continue;
      }

      uint8_t response[64];
      response[0] = MODBUS_SLAVE_ID;
      response[1] = 0x03;
      response[2] = numRegs * 2;

      for (int i = 0; i < numRegs; i++) {
        uint16_t val = holdingRegs[startAddr + i];
        response[3 + i * 2] = val >> 8;
        response[4 + i * 2] = val & 0xFF;
      }

      uint16_t crc = ModRTU_CRC(response, 3 + numRegs * 2);
      response[3 + numRegs * 2] = crc & 0xFF;
      response[4 + numRegs * 2] = crc >> 8;

      sendResponse(response, 5 + numRegs * 2);
    }

    // -------------------------------------------------
    // FUNCTION 0x06: WRITE SINGLE REGISTER
    // -------------------------------------------------
    else if (function == 0x06) {
      uint16_t regAddr = (buffer[2] << 8) | buffer[3];
      uint16_t regValue = (buffer[4] << 8) | buffer[5];

      // Allow writing to configuration registers 10-19
      if (regAddr >= 10 && regAddr < 20) {
        holdingRegs[regAddr] = regValue;

        // If register 10 is written, update air baseline A_air
        if (regAddr == 10)
        {
            float newAir = regValue / 10.0f;
            float voltage = ads.computeVolts(newAir);

            Serial.println();
            Serial.println("========================================");
            Serial.println("Modbus Air Calibration Request");
            Serial.println("========================================");
            Serial.print("ADC Value : ");
            Serial.println(newAir, 2);
            Serial.print("Voltage   : ");
            Serial.print(voltage, 4);
            Serial.println(" V");

            // Validate voltage against 4 mA air range (0.30V - 0.80V)
            if (voltage >= VOLTAGE_AIR_MIN && voltage <= VOLTAGE_AIR_MAX)
            {
                A_air = newAir;
                saveAirBaseline(A_air);

                Serial.println("Calibration accepted.");
                Serial.print("New A_air : ");
                Serial.println(A_air, 2);
            }
            else
            {
                Serial.println("Calibration rejected.");
                Serial.println("Air calibration voltage outside valid 4-20 mA range.");
            }

            Serial.println("========================================");
        }

        // Echo back request frame as success response
        sendResponse(buffer, len);
      } else {
        // Return illegal data address error
        uint8_t errorResp[5];
        errorResp[0] = MODBUS_SLAVE_ID;
        errorResp[1] = 0x86;  // Function + 0x80
        errorResp[2] = 0x02;  // Illegal data address
        uint16_t errCrc = ModRTU_CRC(errorResp, 3);
        errorResp[3] = errCrc & 0xFF;
        errorResp[4] = errCrc >> 8;
        sendResponse(errorResp, 5);
      }
    }

    len = 0;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
