// ============================================================
// 01_Config.ino
// ============================================================
// PURPOSE:
// Centralized System Configuration and Constants Definition Tab.
//
// RESPONSIBILITIES:
// 1. Define hardware GPIO pin mapping for modem, RS485, ADS1115, buzzer, and RGB LED.
// 2. Define cloud server endpoints, ports, topics, tokens, and device labels.
// 3. Define sensor voltage ranges, calibration baseline limits, and EEPROM map.
// 4. Define alert level thresholds and timing intervals.
//
// USED BY:
// Referenced globally across all tasks and peripheral modules.
//
// IMPORTANT:
// Values in this tab represent exact hardware wiring and system parameters.
// Modifying these parameters affects physical peripheral behavior.
// ============================================================

// ============================================================
// 1. QUECTEL EC200U MODEM HARDWARE PINS
// ============================================================
// ESP32-S3 UART1 RX Pin connected to EC200U TX.
#define EC200U_RX_PIN       7

// ESP32-S3 UART1 TX Pin connected to EC200U RX.
#define EC200U_TX_PIN       8

// Modem Power Key (PWRKEY) GPIO pin for hardware power pulse.
#define EC200U_PW_KEY_PIN   10

// Modem Status Indication GPIO pin (HIGH when EC200U is powered ON).
#define EC200U_STATUS_PIN   2

// Aliases for UART modem pins.
#define MODEM_RX EC200U_RX_PIN
#define MODEM_TX EC200U_TX_PIN

// ============================================================
// 2. MQTT BROKER CONFIGURATION
// ============================================================
// Public HiveMQ broker URL for MQTT telemetry.
const char* MQTT_BROKER = "broker.hivemq.com";

// Standard TCP port for unencrypted MQTT.
const int MQTT_PORT = 1883;

// Dedicated MQTT publish topic.
const char* MQTT_TOPIC = "/sensor/topic/soumik";

// ============================================================
// 3. UBIDOTS CLOUD HTTP CONFIGURATION
// ============================================================
// Ubidots API authentication token.
const char *UBIDOTS_TOKEN = "BBUS-R1UePwaJ2wFg2pKlYiArsPsmMWZvzS";

// Industrial Ubidots REST API server host.
const char *UBIDOTS_SERVER = "industrial.api.ubidots.com";

// Standard HTTP port.
const int UBIDOTS_PORT = 80;

// Target Ubidots device label.
const char *DEVICE_LABEL = "wli";

// Target Ubidots variable labels.
const char *VARIABLE_LABEL_LEVEL = "waterlevel";
const char *VARIABLE_LABEL_STATUS = "sensorstatus";

// ============================================================
// 4. RS485 / MODBUS RTU SLAVE CONFIGURATION
// ============================================================
// ESP32-S3 Hardware Serial 2 RX pin connected to MAX485 RO.
#define RS485_RX 16

// ESP32-S3 Hardware Serial 2 TX pin connected to MAX485 DI.
#define RS485_TX 17

// MAX485 Driver Enable / Receiver Enable control pin (DE/RE).
#define RS485_DE 4

// Modbus RTU Slave Identifier (Device Address 1).
#define MODBUS_SLAVE_ID 1

// ============================================================
// 5. EEPROM MEMORY MAP & CALIBRATION LAYOUT
// ============================================================
// Total allocated Non-Volatile EEPROM size in bytes.
#define EEPROM_SIZE 512

// EEPROM memory offset for 32-bit float air baseline ADC value.
#define EEPROM_ADDR_AIR 0

// Magic number validation key to verify valid EEPROM formatting.
#define EEPROM_MAGIC 0xAA55

// EEPROM offset storing the magic number key (2 bytes).
#define EEPROM_ADDR_MAGIC 10

// EEPROM offset storing the calibration status flag (1 byte: 1=Calibrated).
#define EEPROM_ADDR_CALIBRATED 12

// EEPROM offset storing the alarm state flag (1 byte: 1=Enabled, 0=Disabled).
#define EEPROM_ADDR_ALARM_ENABLED 14

// ============================================================
// 6. GPIO & SENSOR PIN DEFINITIONS
// ============================================================
// Piezo Electric Buzzer GPIO pin.
const int BUZZER = 21;

// Status Indicator RGB LED GPIO pin.
const int RGB_LED = 39;

// Ambient temperature sensor input pin.
#define TEMPERATURE_PIN 4

// Default I2C address of ADS1115 16-bit ADC module (ADDR connected to GND).
#define ADS1115_ADDR 0x48

// ============================================================
// 7. BUZZER HARDWARE PWM SETTINGS
// ============================================================
// ESP32 LEDC PWM channel dedicated to buzzer output.
const int BUZZER_CHANNEL = 0;

// Fundamental tone frequency in Hertz (2 kHz).
const int BUZZER_FREQ = 2000;

// PWM duty cycle resolution (8-bit: 0..255).
const int BUZZER_RESOLUTION = 8;

// ============================================================
// 8. ALERT WATER LEVEL THRESHOLDS (CENTIMETERS)
// ============================================================
// Level below 10 cm triggers warning alarm beep & orange indication.
const float LOW_LEVEL_THRESHOLD = 10.0f;

// Level below 5 cm triggers critical alarm pattern & flashing red indication.
const float CRITICAL_LEVEL_THRESHOLD = 5.0f;

// Hysteresis threshold in cm for clearing overflow alarm state (0.5 cm).
const float OVERFLOW_HYSTERESIS_CM = 0.5f;

// ============================================================
// 9. SYSTEM TIMING & INTERVAL CONFIGURATION
// ============================================================
// Cloud telemetry upload frequency (180,000 ms = 180 seconds = 3 minutes).
constexpr unsigned long UBIDOTS_UPLOAD_INTERVAL_MS = 180000UL;
const unsigned long updateFrequency = UBIDOTS_UPLOAD_INTERVAL_MS;


// ADC sensor sampling interval in milliseconds (100 ms).
const unsigned long sensorReadInterval = 100;

// Automatic air recalibration check interval (300,000 ms = 5 minutes).
const unsigned long RECALIBRATE_INTERVAL = 300000UL;

// ============================================================
// 10. SENSOR & TANK CALIBRATION CONSTANTS (4-20 mA SENSOR)
// ============================================================
// Measured zero baseline voltage in air (0.3901 V)
constexpr float SENSOR_ZERO_VOLTAGE = 0.3901f;

// Nominal sensor full-scale electrical specs (0.40 V -> 0 cm, 2.00 V -> 500 cm)
constexpr float SENSOR_FULL_SCALE_VOLTAGE = 2.00f;
constexpr float SENSOR_FULL_SCALE_HEIGHT_CM = 500.0f;

// Physical tank height (50 cm = 100% full capacity)
constexpr float TANK_HEIGHT_CM = 50.0f;

// Empirical sensor calibration factor (Volts per cm of physical water level)
// Derived from physical measurement: (0.4703 V - 0.3901 V) / 28.0 cm = 0.0028643 V/cm
constexpr float VOLTS_PER_CM = 0.0028643f;

// ADC counts per centimeter of physical liquid column (GAIN_ONE: 8000 LSB/V * 0.0028643 V/cm)
const float ADC_PER_CM = 22.9143f;


// Exponential Moving Average (EMA) smoothing coefficient.
const float alpha = 0.1f;

// Number of raw ADC readings to average per sample cycle.
const int SAMPLES = 10;

// Liquid level deadband threshold below which height is forced to 0 cm.
const float DEADBAND_THRESHOLD = 0.5f;

// Voltage threshold below which the 4-20mA loop is declared disconnected.
const float VOLTAGE_DISCONNECT_THRESHOLD = 0.25f;

// Valid voltage bounds during air calibration (0 cm level at 4 mA loop current).
const float VOLTAGE_AIR_MIN = 0.30f;
const float VOLTAGE_AIR_MAX = 0.80f;

// Ideal ADS1115 ADC reading at 0.40 V (GAIN_ONE).
const float IDEAL_AIR_ADC = 3200.0f;

// Number of consecutive invalid voltage readings before setting disconnect flag.
const int DISCONNECTED_THRESHOLD = 5;

// Number of samples taken during auto-recalibration cycles.
const int AIR_READING_SAMPLES = 50;

// ============================================================
// 11. BLE GATT SERVICE & CHARACTERISTIC UUIDS
// ============================================================
// Primary Nordic UART Service UUID.
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

// RX Characteristic UUID (Write commands into ESP32).
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// TX Characteristic UUID (Notify telemetry back to client).
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ============================================================
// 12. HARDWARE DEBUGGING SWITCH
// ============================================================
// Debug switch for observing raw ADS1115 channel-0 readings:
//   0 = Production Mode (default)
//   1 = Raw ADC I2C diagnostic print loop
#define DEBUG_ADC 0
