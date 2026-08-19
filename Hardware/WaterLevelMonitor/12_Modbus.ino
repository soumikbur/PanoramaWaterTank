// ============================================================
// 12_Modbus.ino
// ============================================================
// PURPOSE:
// Modbus RTU Protocol & RS485 Transceiver Interface Tab.
//
// RESPONSIBILITIES:
// 1. Compute 16-bit Modbus RTU Cyclic Redundancy Check (CRC-16 Modbus polynomial 0xA001).
// 2. Control MAX485 direction pin (DE/RE pin 4 HIGH = Transmit, LOW = Receive).
// 3. Transmit Modbus response frames over RS485Serial (Hardware Serial 2 at 9600 baud, 8N1).
//
// HARDWARE CONNECTIONS:
// - RS485 RX: GPIO 16
// - RS485 TX: GPIO 17
// - RS485 DE/RE: GPIO 4
// - Slave ID: 1
//
// USED BY:
// - ModbusTask (uses CRC, direction control, and sendResponse to process PLC requests)
// ============================================================

// ============================================================
// COMPUTE MODBUS RTU CRC-16
// ============================================================
// Calculates standard 16-bit CRC over byte buffer using polynomial 0xA001.
uint16_t ModRTU_CRC(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= buf[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// ============================================================
// RS485 DIRECTION CONTROL: ENABLE TRANSMITTER (TX)
// ============================================================
// Sets DE/RE pin HIGH to enable MAX485 driver outputs.
void enableTX() {
  digitalWrite(RS485_DE, HIGH);
}

// ============================================================
// RS485 DIRECTION CONTROL: ENABLE RECEIVER (RX)
// ============================================================
// Sets DE/RE pin LOW to enable MAX485 receiver input.
void enableRX() {
  digitalWrite(RS485_DE, LOW);
}

// ============================================================
// SEND MODBUS RESPONSE FRAME OVER RS485
// ============================================================
// Enables TX mode, writes frame buffer to RS485 serial, flushes transmit buffer, and returns to RX mode.
void sendResponse(uint8_t *frame, int len) {
  enableTX();
  RS485Serial.write(frame, len);
  RS485Serial.flush();
  enableRX();
}
