// ============================================================
// 02_SharedData.ino
// ============================================================
// PURPOSE:
// Global Shared Data Structures, Variables, and Mutex Allocation Tab.
//
// RESPONSIBILITIES:
// 1. Declare and instantiate system-wide Hardware Serial objects (SerialAT, RS485Serial).
// 2. Instantiate the QuectelEC200U modem and Adafruit ADS1115 ADC driver objects.
// 3. Declare thread-safe volatile shared telemetry variables (Level, Pressure, Status).
// 4. Instantiate FreeRTOS synchronization mutexes (dataMutex, modemMutex).
// 5. Declare task handles for FreeRTOS dual-core multitasking.
//
// USED BY:
// Referenced by all FreeRTOS tasks (SensorTask, UploadTask, MQTTTask, BLETask, ModbusTask, AlertTask).
//
// IMPORTANT:
// Global objects declared here exist exactly once across the entire Arduino sketch.
// Modifying shared variables from multiple tasks MUST be guarded by dataMutex or modemMutex.
// ============================================================

// ============================================================
// 1. HARDWARE SERIAL & MODEM DRIVER OBJECTS
// ============================================================
// Hardware Serial interface 1 dedicated to EC200U cellular modem UART communication.
HardwareSerial SerialAT(1);

// Serial alias used for raw AT command transmission.
#define EC200 SerialAT

// Quectel EC200U driver instance initialized with SerialAT and hardware RX/TX pins.
QuectelEC200U modem(
    SerialAT,
    115200,
    EC200U_RX_PIN,
    EC200U_TX_PIN
);

// ============================================================
// 2. RS485 & MODBUS RTU SLAVE OBJECTS
// ============================================================
// Hardware Serial interface 2 dedicated to RS485 Modbus RTU communication.
HardwareSerial RS485Serial(2);

// Modbus holding register bank (Registers 0 to 19).
uint16_t holdingRegs[20];

// Modbus state tracking variables.
float lastModbusLevel = 0.0f;
float lastModbusPressure = 0.0f;

// ============================================================
// 3. THREAD-SAFE SHARED TELEMETRY VARIABLES
// ============================================================
// Latest calculated liquid level in centimeters (protected by dataMutex).
volatile float sharedLiquidLevel = 0.0f;

// Latest calculated pressure offset in ADC counts (protected by dataMutex).
volatile float sharedPressure = 0.0f;

// Current sensor health status (1 = Normal/Connected, 0 = Disconnected; protected by dataMutex).
volatile int sharedSensorStatus = 0;

// Flag indicating valid sensor measurements are available.
volatile bool dataReady = false;

// Global alarm activation flag (true = Buzzer & RGB LED alerts active; saved in EEPROM).
volatile bool alarmEnabled = true;

// Flag indicating cellular modem power, SIM card, network registration, and PDP context are READY.
volatile bool modemReady = false;

// ============================================================
// 4. FREERTOS MUTEX SYNCHRONIZATION HANDLES
// ============================================================
// Mutex guarding access to shared telemetry variables (sharedLiquidLevel, sharedPressure, sharedSensorStatus).
SemaphoreHandle_t dataMutex = NULL;

// Mutex guarding exclusive access to the EC200U UART serial bus between Ubidots and MQTT tasks.
SemaphoreHandle_t modemMutex = NULL;

// ============================================================
// 5. FREERTOS TASK HANDLES
// ============================================================
// Handle for Core 0 Sensor Acquisition & Filtering Task.
TaskHandle_t SensorTaskHandle = NULL;

// Handle for Core 1 Ubidots Cloud HTTP Upload Task.
TaskHandle_t UploadTaskHandle = NULL;

// Handle for Core 1 Bluetooth Low Energy Server & Diagnostic Task.
TaskHandle_t BLETaskHandle = NULL;

// Handle for Core 1 Alarm Monitoring & Tone Generation Task.
TaskHandle_t AlertTaskHandle = NULL;

// Handle for Core 1 RS485 Modbus RTU Slave Server Task.
TaskHandle_t ModbusTaskHandle = NULL;

// Handle for Core 1 HiveMQ Cellular MQTT Publishing Task.
TaskHandle_t MQTTTaskHandle = NULL;

// ============================================================
// 6. SENSOR ACQUISITION & CALIBRATION STATE VARIABLES
// ============================================================
// Adafruit ADS1115 16-bit I2C ADC driver object.
Adafruit_ADS1115 ads;

// Calibrated 0 cm air baseline ADC value.
float A_air = 0.0f;

// Exponentially filtered ADC measurement.
float filteredADC = 0.0f;

// Timestamp of last Ubidots cloud upload execution.
unsigned long lastUpload = 0;

// Cellular modem 15-digit unique International Mobile Equipment Identity (IMEI) string.
String imei = "";

// Cumulative total count of successful cloud uploads.
int uploadCount = 0;

// Consecutive invalid reading counter for sensor disconnect detection.
int badReadingCount = 0;

// Flag indicating physical sensor disconnection.
bool sensorDisconnected = false;

// Timestamp of last valid ADC reading.
unsigned long lastGoodReading = 0;

// Timestamp of last automatic air baseline recalibration cycle.
unsigned long lastRecalibration = 0;

// ============================================================
// 7. BLUETOOTH LOW ENERGY (BLE) STATE VARIABLES
// ============================================================
// Pointer to BLE Server instance.
BLEServer *pServer = NULL;

// Pointer to BLE Transmit Characteristic (Notifications to client).
BLECharacteristic *pTxCharacteristic = NULL;

// Flag indicating an active BLE client connection.
bool deviceConnected = false;

// Previous BLE client connection state for disconnect/re-advertising tracking.
bool oldDeviceConnected = false;
