// ============================================================
// 04_ADS1115.ino
// ============================================================
// PURPOSE:
// Texas Instruments ADS1115 16-bit Precision I2C ADC Operations Tab.
//
// RESPONSIBILITIES:
// 1. Interface with ADS1115 ADC over I2C bus (SDA pin 14, SCL pin 15).
// 2. Perform voltage conversion using computeVolts() at GAIN_ONE (+/-4.096V range).
// 3. Perform 4-20 mA current loop sensor connectivity detection.
//
// HARDWARE PARAMETERS:
// - ADS1115 I2C Address: 0x48
// - Gain: GAIN_ONE (+/-4.096V full scale, 0.125mV/bit)
// - Disconnect Voltage Threshold: 0.25 V
//   (0 mA = 0.00V -> Disconnected; 4 mA = ~0.40V -> Air baseline)
//
// USED BY:
// - SensorTask (reads channel 0, computes voltage, checks connection state)
// ============================================================

// ============================================================
// CHECK SENSOR CONNECTION VIA ADC VOLTAGE
// ============================================================
// Converts ADC counts into voltage and checks against the disconnect threshold (0.25 V).
// Returns true if sensor is connected (voltage >= 0.25V), false if disconnected (0 V).
bool isSensorConnected(float adcValue)
{
    // Convert raw ADC reading to voltage in Volts.
    float voltage = ads.computeVolts(adcValue);

    // Print raw ADC and calculated voltage to Serial Monitor for diagnostics.
    Serial.print("ADC      : ");
    Serial.print(adcValue, 2);
    Serial.print("   Voltage : ");
    Serial.print(voltage, 4);
    Serial.println(" V");

    // A disconnected 4–20 mA pressure transducer produces ~0 V.
    // Any reading above 0.25 V indicates an active loop current.
    if (voltage >= VOLTAGE_DISCONNECT_THRESHOLD)
    {
        return true;
    }

    return false;
}
