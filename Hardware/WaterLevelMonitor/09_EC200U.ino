// ============================================================
// 09_EC200U.ino
// ============================================================
// PURPOSE:
// Low-Level Quectel EC200U LTE Cat-1 Modem Communication & Driver Tab.
//
// RESPONSIBILITIES:
// 1. Hardware power control (PWRKEY pin 10 toggle sequence & status check).
// 2. High-reliability AT command transmission over Hardware Serial 1 (SerialAT).
// 3. Multi-stage asynchronous URC response line parsing (waitForURC, waitForModemResponse).
// 4. Send prompt wait helper (waitForSendPrompt for '>' character).
// 5. Idle serial buffer reader (readUntilIdle).
// 6. Modem hardware initialization (ATE0 echo disable, SIM CPIN status check).
// 7. PDP context activation and verification (AT+QIACT=1).
// 8. 15-digit modem IMEI reading (AT+CGSN).
// 9. TCP socket cleanup on Socket 1 (AT+QICLOSE=1).
//
// USED BY:
// - UploadTask (relies on modem initialization, socket 1 cleanup, PDP context)
// - MQTTTask (relies on modem initialization, PDP context)
//
// IMPORTANT:
// Modifying modem timeouts or AT command strings directly affects cellular stability.
// Access to modem serial commands from background tasks MUST acquire modemMutex.
// ============================================================

// ============================================================
// FLUSH STALE DATA FROM MODEM RX BUFFER
// ============================================================
void flushModemRx()
{
    while (EC200.available())
    {
        EC200.read();
    }
}

// ============================================================
// SEND AT COMMAND AND WAIT FOR EXPECTED RESPONSE
// ============================================================
// Sends an AT command string to EC200U modem over UART and collects response until expected string arrives or timeout.
bool sendAT(const String &cmd, const String &expect, unsigned long timeout)
{
    flushModemRx();

    Serial.println();
    Serial.println(">> " + cmd);

    EC200.println(cmd);

    String response;
    response.reserve(512);

    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        while (EC200.available())
        {
            char c = (char)EC200.read();

            response += c;
            Serial.write(c);

            if (expect.length() > 0 && response.indexOf(expect) >= 0)
            {
                return true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    Serial.println();
    Serial.println("!! AT TIMEOUT");
    Serial.println("Expected: " + expect);

    return false;
}

// ============================================================
// RECEIVE-ONLY RESPONSE WAIT HELPER
// ============================================================
// Waits for expected response string without transmitting trailing CR/LF characters over UART.
bool waitForModemResponse(const String &expect, unsigned long timeout, String &collected)
{
    collected = "";

    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        while (EC200.available())
        {
            char c = (char)EC200.read();

            collected += c;
            Serial.write(c);

            if (expect.length() > 0 && collected.indexOf(expect) >= 0)
            {
                return true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return false;
}

// ============================================================
// WAIT FOR EC200U SEND PROMPT ('>')
// ============================================================
// Waits for modem to return '>' prompt character during TCP send (QISEND / QMTPUB).
bool waitForSendPrompt(unsigned long timeout)
{
    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        while (EC200.available())
        {
            char c = (char)EC200.read();

            Serial.write(c);

            if (c == '>')
            {
                return true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    Serial.println();
    Serial.println("!! EC200U send prompt timeout");

    return false;
}

// ============================================================
// READ MODEM RESPONSE UNTIL BUS BECOMES IDLE
// ============================================================
// Continually accumulates data until idleGap milliseconds of silence occur after receiving bytes.
void readUntilIdle(unsigned long maxTotal, unsigned long idleGap, String &collected)
{
    collected = "";

    unsigned long start = millis();
    unsigned long lastByte = start;

    while (millis() - start < maxTotal)
    {
        bool received = false;

        while (EC200.available())
        {
            char c = (char)EC200.read();

            collected += c;
            Serial.write(c);

            received = true;
        }

        if (received)
        {
            lastByte = millis();
        }
        else if (collected.length() > 0 && millis() - lastByte >= idleGap)
        {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ============================================================
// EC200U HARDWARE POWER ON SEQUENCE
// ============================================================
// Checks modem status pin. If modem is OFF, pulses PWRKEY pin LOW for 2 seconds to initiate boot.
void EC200U_powerOn()
{
    pinMode(EC200U_PW_KEY_PIN, OUTPUT);
    pinMode(EC200U_STATUS_PIN, INPUT);

    // Keep PWRKEY inactive (HIGH).
    digitalWrite(EC200U_PW_KEY_PIN, HIGH);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Check modem power status pin.
    if (digitalRead(EC200U_STATUS_PIN) == LOW)
    {
        Serial.println("EC200U is OFF.");
        Serial.println("Powering ON...");

        // Pull PWRKEY LOW for 2 seconds to turn ON modem power.
        digitalWrite(EC200U_PW_KEY_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Return PWRKEY to HIGH.
        digitalWrite(EC200U_PW_KEY_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    else
    {
        Serial.println("EC200U is already ON.");
    }
}

// ============================================================
// WAIT FOR ASYNCHRONOUS URC RESPONSE LINE WITH NEWLINE
// ============================================================
// Collects incoming serial data until the specified tag is found AND a complete line ending (\n) is received.
bool waitForURC(const String &tag, unsigned long timeout, String &collected)
{
    collected = "";
    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        while (EC200.available())
        {
            char c = (char)EC200.read();
            collected += c;
            Serial.write(c);

            int tagPos = collected.indexOf(tag);
            if (tagPos >= 0 && collected.indexOf('\n', tagPos) >= 0)
            {
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return (collected.indexOf(tag) >= 0);
}

// ============================================================
// READ EC200U 15-DIGIT IMEI NUMBER
// ============================================================
// Issues AT+CGSN command and extracts 15-digit unique cellular device IMEI.
String readIMEI()
{
    flushModemRx();

    Serial.println();
    Serial.println(">> AT+CGSN");

    EC200.println("AT+CGSN");

    String response;
    response.reserve(128);

    unsigned long start = millis();

    while (millis() - start < 2000)
    {
        while (EC200.available())
        {
            char c = (char)EC200.read();

            response += c;
            Serial.write(c);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Extract first valid 15-digit numeric sequence.
    String imeiResult;

    for (int i = 0; i < response.length();)
    {
        if (!isDigit(response[i]))
        {
            i++;
            continue;
        }

        String candidate;

        while (i < response.length() && isDigit(response[i]))
        {
            candidate += response[i];
            i++;
        }

        if (candidate.length() == 15)
        {
            imeiResult = candidate;
            break;
        }
    }

    if (imeiResult.length() == 15)
    {
        Serial.println();
        Serial.println("IMEI: " + imeiResult);
    }
    else
    {
        Serial.println();
        Serial.println("!! Failed to read valid IMEI");
    }

    return imeiResult;
}

// ============================================================
// ENSURE PDP CONTEXT ACTIVATION (PDP CONTEXT 1)
// ============================================================
// Queries PDP context status (AT+QIACT?). Activates PDP Context 1 if not active (AT+QIACT=1).
bool ensurePDP()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("Checking PDP Context...");
    Serial.println("========================================");

    while (EC200.available())
    {
        EC200.read();
    }

    EC200.println("AT+QIACT?");

    String response = "";
    unsigned long start = millis();

    while (millis() - start < 3000)
    {
        while (EC200.available())
        {
            response += (char)EC200.read();
        }

        if (response.indexOf("OK") >= 0)
        {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Serial.println("[PDP] Query response:");
    Serial.println(response);

    bool pdpActive = false;
    int qiactIndex = response.indexOf("+QIACT:");

    if (qiactIndex >= 0)
    {
        String qiactLine = response.substring(qiactIndex);
        int lineEnd = qiactLine.indexOf('\n');

        if (lineEnd >= 0)
        {
            qiactLine = qiactLine.substring(0, lineEnd);
        }

        Serial.print("[PDP] Context information: ");
        Serial.println(qiactLine);

        if (qiactLine.indexOf("+QIACT: 1,1,1") >= 0)
        {
            pdpActive = true;
        }
    }

    if (pdpActive)
    {
        Serial.println("[PDP] Context 1 is already active.");
        return true;
    }

    Serial.println("[PDP] Context 1 is not active.");
    Serial.println("[PDP] Activating PDP context...");

    if (!sendAT("AT+QIACT=1", "OK", 15000))
    {
        Serial.println("[PDP] ERROR: Failed to activate PDP context.");
        return false;
    }

    Serial.println("[PDP] QIACT command accepted.");
    vTaskDelay(pdMS_TO_TICKS(500));

    while (EC200.available())
    {
        EC200.read();
    }

    EC200.println("AT+QIACT?");

    response = "";
    start = millis();

    while (millis() - start < 3000)
    {
        while (EC200.available())
        {
            response += (char)EC200.read();
        }

        if (response.indexOf("OK") >= 0)
        {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Serial.println("[PDP] Verification response:");
    Serial.println(response);

    qiactIndex = response.indexOf("+QIACT:");

    if (qiactIndex >= 0)
    {
        String qiactLine = response.substring(qiactIndex);
        int lineEnd = qiactLine.indexOf('\n');

        if (lineEnd >= 0)
        {
            qiactLine = qiactLine.substring(0, lineEnd);
        }

        if (qiactLine.indexOf("+QIACT: 1,1,1") >= 0)
        {
            Serial.println("[PDP] PDP context 1 activated successfully.");
            return true;
        }
    }

    Serial.println("[PDP] ERROR: PDP activation could not be verified.");
    return false;
}

// ============================================================
// EC200U MODEM INITIALIZATION SEQUENCE
// ============================================================
// Performs full hardware initialization: UART start, ATE0, SIM status check, PDP context setup, and IMEI reading.
bool connectModem()
{
    modemReady = false;

    Serial.println();
    Serial.println("========================================");
    Serial.println("Initializing EC200U...");
    Serial.println("========================================");

    Serial.println("Starting EC200U UART...");

    EC200.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(3000);

    flushModemRx();

    Serial.println();
    Serial.println("Checking modem...");

    if (!sendAT("AT", "OK", 3000))
    {
        Serial.println("ERROR: EC200U did not respond to AT.");
        return false;
    }

    Serial.println("Modem communication : OK");

    Serial.println("Disabling modem echo...");

    if (!sendAT("ATE0", "OK", 3000))
    {
        Serial.println("ERROR: Failed to disable modem echo.");
        return false;
    }

    Serial.println("Modem echo : DISABLED");

    Serial.println();
    Serial.println("Checking SIM card...");

    if (!sendAT("AT+CPIN?", "READY", 5000))
    {
        Serial.println("ERROR: SIM card is not ready.");
        return false;
    }

    Serial.println("SIM status : READY");

    Serial.println();
    Serial.println("Checking network registration...");

    if (!sendAT("AT+CREG?", "OK", 5000))
    {
        Serial.println("WARNING: CREG query failed.");
    }

    if (!sendAT("AT+CGREG?", "OK", 5000))
    {
        Serial.println("WARNING: CGREG query failed.");
    }

    if (!sendAT("AT+CEREG?", "OK", 5000))
    {
        Serial.println("WARNING: CEREG query failed.");
    }

    Serial.println();
    Serial.println("Checking signal quality...");

    if (!sendAT("AT+CSQ", "OK", 3000))
    {
        Serial.println("WARNING: CSQ query failed.");
    }

    Serial.println();
    Serial.println("Checking PDP context...");

    if (!ensurePDP())
    {
        Serial.println("ERROR: PDP context is not available.");
        return false;
    }

    Serial.println("PDP context : READY");

    Serial.println("Initializing QuectelEC200U modem...");
    modem.begin();

    Serial.println();
    Serial.println("Reading IMEI...");

    imei = readIMEI();

    if (imei.length() == 0)
    {
        Serial.println("ERROR: Failed to read EC200U IMEI.");
        return false;
    }

    Serial.print("IMEI : ");
    Serial.println(imei);

    modemReady = true;

    Serial.println();
    Serial.println("========================================");
    Serial.println("EC200U INITIALIZATION COMPLETE");
    Serial.println("========================================");
    Serial.println("UART              : READY");
    Serial.println("SIM               : READY");
    Serial.println("Network           : CHECKED");
    Serial.println("PDP Context       : READY");
    Serial.println("MQTT TLS          : CONFIGURED");
    Serial.println("MQTT Connection   : NOT CONNECTED");
    Serial.print("IMEI              : ");
    Serial.println(imei);
    Serial.println("Modem Ready       : YES");
    Serial.println("========================================");

    return true;
}

// ============================================================
// CLOSE TCP SOCKET (SOCKET 1)
// ============================================================
// Closes TCP Socket 1 used by Ubidots HTTP operations.
void closeSocket()
{
    Serial.println("[HTTP] Closing TCP socket 1...");

    sendAT("AT+QICLOSE=1", "OK", 3000);

    vTaskDelay(pdMS_TO_TICKS(300));

    while (EC200.available())
    {
        EC200.read();
    }

    Serial.println("[HTTP] TCP socket cleanup complete.");
}
