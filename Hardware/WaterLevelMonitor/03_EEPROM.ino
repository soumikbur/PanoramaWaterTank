// ============================================================
// 03_EEPROM.ino
// ============================================================
// PURPOSE:
// Non-Volatile EEPROM Persistence and Configuration Storage Tab.
//
// RESPONSIBILITIES:
// 1. Persist calibrated air baseline ADC value (A_air) across power reboots.
// 2. Validate stored EEPROM formatting using a 16-bit magic number key (0xAA55).
// 3. Persist and restore user alarm state (Enabled/Disabled).
//
// USED BY:
// - SensorTask (loads calibration & alarm state at boot, saves auto-recalibrations)
// - MyCallbacks (BLE commands save alarm state & manual air calibration)
// - ModbusTask (Modbus write register 10 saves air baseline)
//
// IMPORTANT:
// EEPROM memory layout is strictly preserved:
// Offset  0: float air baseline
// Offset 10: uint16_t magic number (0xAA55)
// Offset 12: uint8_t calibration valid flag (1 = Valid)
// Offset 14: uint8_t alarm enabled flag (1 = Enabled, 0 = Disabled)
// ============================================================

// ============================================================
// SAVE AIR BASELINE TO EEPROM
// ============================================================
// Stores the measured zero-level air baseline ADC value into non-volatile EEPROM.
void saveAirBaseline(float airValue)
{
    // Initialize EEPROM flash emulation storage.
    EEPROM.begin(EEPROM_SIZE);

    // Write magic number key to confirm valid EEPROM formatting.
    uint16_t magic = EEPROM_MAGIC;
    EEPROM.put(EEPROM_ADDR_MAGIC, magic);

    // Write 32-bit float air baseline ADC value.
    EEPROM.put(EEPROM_ADDR_AIR, airValue);

    // Set calibration valid flag to 1.
    EEPROM.put(EEPROM_ADDR_CALIBRATED, (uint8_t)1);

    // Commit changes to flash memory.
    EEPROM.commit();

    // Close EEPROM storage session.
    EEPROM.end();

    Serial.print(" Saved A_air to EEPROM: ");
    Serial.println(airValue, 2);
}

// ============================================================
// CHECK IF EEPROM CONTAINS VALID CALIBRATION DATA
// ============================================================
// Reads magic number and calibration flag to verify if valid air calibration exists.
bool isCalibrated()
{
    EEPROM.begin(EEPROM_SIZE);

    uint16_t magic;
    EEPROM.get(EEPROM_ADDR_MAGIC, magic);

    uint8_t calibrated;
    EEPROM.get(EEPROM_ADDR_CALIBRATED, calibrated);

    EEPROM.end();

    // Return true only if magic number matches 0xAA55 and calibration flag is 1.
    return (magic == EEPROM_MAGIC && calibrated == 1);
}

// ============================================================
// LOAD AIR BASELINE FROM EEPROM
// ============================================================
// Retrieves stored air baseline ADC value and validates voltage limits (0.30V - 0.80V).
float loadAirBaseline()
{
    EEPROM.begin(EEPROM_SIZE);

    uint16_t magic;
    EEPROM.get(EEPROM_ADDR_MAGIC, magic);

    if (magic == EEPROM_MAGIC)
    {
        float savedAir;
        EEPROM.get(EEPROM_ADDR_AIR, savedAir);

        EEPROM.end();

        // Compute corresponding voltage for saved ADC value at GAIN_ONE.
        float voltage = ads.computeVolts(savedAir);

        // Validate that stored baseline corresponds to valid 4 mA air voltage.
        if (voltage >= VOLTAGE_AIR_MIN && voltage <= VOLTAGE_AIR_MAX)
        {
            Serial.print(" Loaded A_air from EEPROM: ");
            Serial.println(savedAir, 2);
            return savedAir;
        }

        Serial.println(" Saved calibration out of valid range");
    }

    EEPROM.end();

    // Return 0.0f if EEPROM is uncalibrated or out of range.
    return 0.0f;
}

// ============================================================
// SAVE ALARM ENABLED/DISABLED STATE TO EEPROM
// ============================================================
// Persists alarm preference (true = Enabled, false = Disabled) to EEPROM.
void saveAlarmState(bool state)
{
    EEPROM.begin(EEPROM_SIZE);

    // Store state as uint8_t byte (1 = Enabled, 0 = Disabled).
    EEPROM.put(EEPROM_ADDR_ALARM_ENABLED, (uint8_t)(state ? 1 : 0));

    EEPROM.commit();
    EEPROM.end();

    Serial.print(" Saved alarm state to EEPROM: ");
    Serial.println(state ? "ENABLED" : "DISABLED");
}

// ============================================================
// LOAD ALARM ENABLED/DISABLED STATE FROM EEPROM
// ============================================================
// Reads stored alarm preference from EEPROM. Defaults to Enabled if uninitialized.
bool loadAlarmState()
{
    EEPROM.begin(EEPROM_SIZE);

    uint8_t savedState;
    EEPROM.get(EEPROM_ADDR_ALARM_ENABLED, savedState);

    EEPROM.end();

    // If memory is uninitialized, default to ENABLED (true) and save.
    if (savedState != 0 && savedState != 1)
    {
        saveAlarmState(true);
        return true;
    }

    return (savedState == 1);
}
