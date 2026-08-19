// ============================================================
// 10_MQTT.ino
// ============================================================
// PURPOSE:
// HiveMQ MQTT Protocol Client Implementation Tab.
//
// RESPONSIBILITIES:
// 1. Reset and close previous MQTT Client 0 sessions (AT+QMTDISC=0, AT+QMTCLOSE=0).
// 2. Open MQTT network socket to broker (AT+QMTOPEN=0,"broker.hivemq.com",1883).
// 3. Connect to MQTT broker using unique client ID incorporating modem IMEI (AT+QMTCONN=0,"ESP32_WLI_<IMEI>").
// 4. Publish telemetry JSON payloads to target topic (/sensor/topic/soumik) using AT+QMTPUB.
// 5. Handle async response URC parsing (+QMTOPEN, +QMTCONN, +QMTPUB).
//
// USED BY:
// - MQTTTask (executes connectMQTT and publishMQTT under modemMutex protection)
// ============================================================

// ============================================================
// CONNECT TO MQTT BROKER (HIVEMQ)
// ============================================================
// Resets MQTT Client 0, opens socket, and establishes connection with unique IMEI client ID.
bool connectMQTT()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("Connecting to MQTT (HiveMQ)...");
    Serial.println("========================================");

    // 1. Flush any leftover serial data
    while (EC200.available())
    {
        EC200.read();
    }

    // 2. Disconnect and close any previous MQTT session on Client 0
    Serial.println("Resetting MQTT Client 0...");
    EC200.println("AT+QMTDISC=0");
    vTaskDelay(pdMS_TO_TICKS(300));
    while (EC200.available()) EC200.read();

    EC200.println("AT+QMTCLOSE=0");
    vTaskDelay(pdMS_TO_TICKS(500));
    while (EC200.available()) EC200.read();

    // 3. Issue AT+QMTOPEN=0,"broker.hivemq.com",1883
    String openCmd = "AT+QMTOPEN=0,\"" + String(MQTT_BROKER) + "\"," + String(MQTT_PORT);
    Serial.println("Opening MQTT network socket...");
    Serial.print(">> ");
    Serial.println(openCmd);
    EC200.println(openCmd);

    // Wait until +QMTOPEN: URC line arrives complete with newline
    String openResp;
    bool openOk = waitForURC("+QMTOPEN:", 15000, openResp);

    if (!openOk || (openResp.indexOf("+QMTOPEN: 0,0") < 0 && openResp.indexOf("+QMTOPEN: 0,2") < 0))
    {
        Serial.println("\r\nERROR: MQTT network connection (+QMTOPEN) failed.");
        return false;
    }

    Serial.println("\r\nMQTT network connection OPEN.");

    // 4. Generate unique Client ID using IMEI to avoid broker conflicts
    String clientID = "ESP32_WLI_";
    if (imei.length() >= 15)
    {
        clientID += imei;
    }
    else
    {
        clientID += String(random(10000, 99999));
    }

    String connCmd = "AT+QMTCONN=0,\"" + clientID + "\"";
    Serial.println("Connecting to MQTT Broker...");
    Serial.print(">> ");
    Serial.println(connCmd);
    EC200.println(connCmd);

    // Wait until +QMTCONN: URC line arrives complete with newline
    String connResp;
    bool connOk = waitForURC("+QMTCONN:", 10000, connResp);

    if (connOk && (connResp.indexOf("+QMTCONN: 0,0,0") >= 0 || connResp.indexOf("+QMTCONN: 0,0") >= 0))
    {
        Serial.println("\r\n========================================");
        Serial.println("MQTT CONNECTED SUCCESSFULLY!");
        Serial.print("Client ID : ");
        Serial.println(clientID);
        Serial.print("Broker    : ");
        Serial.println(MQTT_BROKER);
        Serial.print("Topic     : ");
        Serial.println(MQTT_TOPIC);
        Serial.println("========================================");
        return true;
    }

    Serial.println("\r\nERROR: MQTT broker connect (+QMTCONN) failed.");
    return false;
}

// ============================================================
// PUBLISH TELEMETRY PAYLOAD TO MQTT TOPIC
// ============================================================
// Issues AT+QMTPUB=0,0,0,0,"/sensor/topic/soumik", writes JSON payload + Ctrl+Z (0x1A), and verifies URC response.
bool publishMQTT(const char* payload)
{
    // Flush serial buffer
    while (EC200.available())
    {
        EC200.read();
    }

    String pubCmd = "AT+QMTPUB=0,0,0,0,\"" + String(MQTT_TOPIC) + "\"";
    Serial.print(">> ");
    Serial.println(pubCmd);
    EC200.println(pubCmd);

    // Wait for '>' prompt
    if (!waitForSendPrompt(5000))
    {
        Serial.println("\r\nERROR: MQTT publish failed - '>' prompt not received.");
        return false;
    }

    // Write JSON payload + Ctrl+Z (0x1A)
    EC200.print(payload);
    EC200.write(0x1A);

    // Wait for OK or +QMTPUB: URC line
    String pubResp;
    bool pubOk = waitForURC("+QMTPUB:", 5000, pubResp);

    if (!pubOk)
    {
        pubOk = (pubResp.indexOf("OK") >= 0);
    }

    if (pubOk && pubResp.indexOf("ERROR") < 0)
    {
        Serial.println("\r\nMQTT publish SUCCESSFUL.");
        return true;
    }

    Serial.println("\r\nERROR: MQTT publish failed.");
    return false;
}
