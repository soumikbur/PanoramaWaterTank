// ============================================================
// 08_BLE.ino
// ============================================================
// PURPOSE:
// Bluetooth Low Energy (BLE) Server Callbacks and ASCII Command Parser Tab.
//
// RESPONSIBILITIES:
// 1. Handle BLE client connect and disconnect lifecycle events.
// 2. Parse incoming ASCII commands over BLE RX characteristic (UUID 6E400002).
// 3. Execute remote configuration commands:
//    - ALARM ON / ALARM_ON
//    - ALARM OFF / ALARM_OFF
//    - ALARM STATUS / ALARM_STATUS
//    - SET_AIR=<ADC_VALUE>
//    - SHOW_AIR
//    - SHOW_LEVEL
//    - HELP / help
// 4. Transmit responses back to client over BLE TX notification characteristic (UUID 6E400003).
//
// USED BY:
// - BLETask (initializes GATT server, registers callbacks, starts advertising)
// ============================================================

// ============================================================
// BLE SERVER CONNECT/DISCONNECT LIFECYCLE CALLBACKS
// ============================================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println(" BLE Device connected");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println(" BLE Device disconnected");
  }
};

// ============================================================
// BLE RX CHARACTERISTIC WRITE CALLBACK & COMMAND PARSER
// ============================================================
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {

    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() == 0)
      return;

    Serial.println(" BLE Received: " + rxValue);

    // -------------------------------------------------
    // 1. ALARM CONTROL COMMANDS
    // -------------------------------------------------

    // Enable Alarm
    if (rxValue == "ALARM ON" || rxValue == "ALARM_ON") {
      alarmEnabled = true;
      saveAlarmState(true);

      String response = "Alarm ENABLED";
      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();

      Serial.println(" Alarm ENABLED via BLE");
    }

    // Disable Alarm
    else if (rxValue == "ALARM OFF" || rxValue == "ALARM_OFF") {
      alarmEnabled = false;
      saveAlarmState(false);

      // Silence buzzer immediately when alarm is disabled.
      ledcWrite(BUZZER, 0);

      String response = "Alarm DISABLED";
      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();

      Serial.println(" Alarm DISABLED via BLE");
    }

    // Query Alarm Status
    else if (rxValue == "ALARM STATUS" || rxValue == "ALARM_STATUS") {
      String response = "Alarm is " + String(alarmEnabled ? "ENABLED" : "DISABLED");

      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();
    }

    // -------------------------------------------------
    // 2. MANUAL AIR BASELINE CALIBRATION COMMAND
    // -------------------------------------------------

    else if (rxValue.startsWith("SET_AIR=")) {
      float newAir = rxValue.substring(8).toFloat();
      float voltage = ads.computeVolts(newAir);

      Serial.println();
      Serial.println("========================================");
      Serial.println("BLE Air Calibration");
      Serial.println("========================================");
      Serial.print("ADC Value : ");
      Serial.println(newAir, 2);
      Serial.print("Voltage   : ");
      Serial.print(voltage, 4);
      Serial.println(" V");

      // Validate corresponding voltage against 4 mA air range (0.30V - 0.80V).
      if (voltage >= VOLTAGE_AIR_MIN && voltage <= VOLTAGE_AIR_MAX) {
        A_air = newAir;
        saveAirBaseline(A_air);

        Serial.println("Calibration accepted.");

        String response = "A_air set to: " + String(A_air, 2);
        pTxCharacteristic->setValue(response.c_str());
        pTxCharacteristic->notify();
      } else {
        Serial.println("Calibration rejected.");
        Serial.println("Outside valid 4-20mA air range.");

        String response = "Invalid air calibration";
        pTxCharacteristic->setValue(response.c_str());
        pTxCharacteristic->notify();
      }

      Serial.println("========================================");
    }

    // -------------------------------------------------
    // 3. SHOW CURRENT AIR BASELINE
    // -------------------------------------------------

    else if (rxValue == "SHOW_AIR") {
      String response = "A_air: " + String(A_air, 2);

      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();
    }

    // -------------------------------------------------
    // 4. SHOW CURRENT WATER LEVEL
    // -------------------------------------------------

    else if (rxValue == "SHOW_LEVEL") {
      String response = "Level: " + String(sharedLiquidLevel, 2) + " cm";

      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();
    }

    // -------------------------------------------------
    // 5. BLE HELP COMMAND
    // -------------------------------------------------

    else if (rxValue == "HELP" || rxValue == "help") {
      String response = "Commands:\n";
      response += "ALARM ON\n";
      response += "ALARM OFF\n";
      response += "ALARM STATUS\n";
      response += "SET_AIR=<ADC_VALUE>\n";
      response += "SHOW_AIR\n";
      response += "SHOW_LEVEL";

      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();
    }
  }
};
