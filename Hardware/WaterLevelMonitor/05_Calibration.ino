// ============================================================
// 05_Calibration.ino
// ============================================================
// PURPOSE:
// Air Baseline Calibration and Automatic Recalibration Tab.
//
// RESPONSIBILITIES:
// 1. Perform initial startup air calibration if no valid EEPROM calibration exists.
// 2. Sample ADS1115 channel 0 for 50 readings in air condition to compute average A_air.
// 3. Validate calculated baseline against valid 4 mA voltage window (0.30V - 0.80V).
// 4. Perform background auto-recalibration every 5 minutes when sensor is in air condition.
//
// CALIBRATION SEQUENCE:
// Check EEPROM → Use saved calibration if valid → Otherwise wait 5 seconds →
// Take 50 samples → Compute average → Convert to voltage → Validate (0.30V..0.80V) → Save to EEPROM.
//
// USED BY:
// - SensorTask (invokes startup calibration during initialization and periodic auto-recalibration)
// ============================================================

// ============================================================
// CALIBRATE AIR BASELINE ON STARTUP
// ============================================================
// Loads EEPROM calibration or performs a 50-sample auto-calibration in air condition.
float calibrateAirOnStartup()
{
    // -------------------------------------------------
    // 1. Check for previously saved EEPROM calibration
    // -------------------------------------------------
    if (isCalibrated())
    {
        float savedAir = loadAirBaseline();

        if (savedAir > 0.0f)
        {
            Serial.println();
            Serial.println("Using saved calibration from EEPROM");
            Serial.print("A_air : ");
            Serial.println(savedAir, 2);

            return savedAir;
        }
    }

    // -------------------------------------------------
    // 2. Start automatic air calibration sequence
    // -------------------------------------------------
    Serial.println();
    Serial.println("No saved calibration found!");
    Serial.println("Auto-calibrating air baseline...");
    Serial.println("Keep sensor in AIR for 5 seconds");

    // 5-second user countdown timer.
    for (int i = 5; i > 0; i--)
    {
        Serial.print(i);
        Serial.println(" seconds...");
        delay(1000);
    }

    // -------------------------------------------------
    // 3. Collect 50 ADC samples
    // -------------------------------------------------
    long sum = 0;
    int validSamples = 0;

    for (int i = 0; i < AIR_READING_SAMPLES; i++)
    {
        int16_t reading = ads.readADC_SingleEnded(0);

        if (reading > 0 && reading < 32767)
        {
            sum += reading;
            validSamples++;
        }

        delay(20);
    }

    // -------------------------------------------------
    // 4. Validate sample count
    // -------------------------------------------------
    if (validSamples == 0)
    {
        Serial.println("ERROR: Air calibration failed - no valid ADC samples.");
        return 0.0f;
    }

    // -------------------------------------------------
    // 5. Calculate average air baseline ADC value
    // -------------------------------------------------
    float airBaseline = (float)sum / (float)validSamples;
    float airVoltage = ads.computeVolts((int16_t)airBaseline);

    Serial.println();
    Serial.println("----------------------------------------");
    Serial.println("AIR CALIBRATION RESULT");
    Serial.println("----------------------------------------");
    Serial.print("Measured Air ADC : ");
    Serial.println(airBaseline, 2);
    Serial.print("Measured Voltage : ");
    Serial.print(airVoltage, 4);
    Serial.println(" V");
    Serial.print("Valid Samples    : ");
    Serial.println(validSamples);

    // -------------------------------------------------
    // 6. Validate measured air voltage (4 mA window)
    // -------------------------------------------------
    if (airVoltage < VOLTAGE_AIR_MIN || airVoltage > VOLTAGE_AIR_MAX)
    {
        Serial.println("ERROR: Air calibration rejected.");
        Serial.println("Measured voltage is outside the expected air range.");
        return 0.0f;
    }

    // -------------------------------------------------
    // 7. Save validated baseline to EEPROM
    // -------------------------------------------------
    Serial.println("Air calibration successful.");
    Serial.print("Air Baseline : ");
    Serial.println(airBaseline, 2);

    saveAirBaseline(airBaseline);

    Serial.println("Air baseline saved to EEPROM.");
    Serial.println("----------------------------------------");

    return airBaseline;
}

// ============================================================
// AUTOMATIC AIR BASELINE RECALIBRATION
// ============================================================
// Periodically updates air baseline if current reading remains near zero water level.
void updateAirBaseline()
{
    float currentReading = filteredADC;
    float baselineDiff = abs(currentReading - A_air);

    // Perform auto-recalibration only if current ADC is within 50 counts of A_air (near zero level).
    if (baselineDiff < 50)
    {
        long sum = 0;

        for (int i = 0; i < AIR_READING_SAMPLES; i++)
        {
            sum += abs(ads.readADC_SingleEnded(0));
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        float newAir = (float)sum / (float)AIR_READING_SAMPLES;
        float voltage = ads.computeVolts(newAir);

        // Accept baseline update if voltage is within valid 4 mA air range (0.30V - 0.80V).
        if (voltage >= VOLTAGE_AIR_MIN && voltage <= VOLTAGE_AIR_MAX)
        {
            if (abs(newAir - A_air) > 5)
            {
                Serial.println("========================================");
                Serial.println("AUTO RECALIBRATION");
                Serial.println("========================================");
                Serial.print("Old A_air : ");
                Serial.println(A_air, 2);
                Serial.print("New A_air : ");
                Serial.println(newAir, 2);
                Serial.print("Voltage   : ");
                Serial.print(voltage, 4);
                Serial.println(" V");

                A_air = newAir;
                saveAirBaseline(A_air);
            }
        }
        else
        {
            Serial.println("Auto calibration skipped.");
            Serial.println("Reading outside valid air voltage range.");
        }
    }
}
