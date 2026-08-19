// ============================================================
// 11_Ubidots.ino
// ============================================================
// PURPOSE:
// Ubidots Cloud HTTP REST API Client Implementation Tab.
//
// RESPONSIBILITIES:
// 1. Construct JSON telemetry payload containing water level, pressure, and sensor status.
// 2. Format complete HTTP/1.1 POST request headers (Host, X-Auth-Token, Content-Type, Content-Length).
// 3. Open TCP Socket 1 to industrial.api.ubidots.com:80 (AT+QIOPEN=1,1,"TCP",...).
// 4. Send HTTP payload over TCP socket (AT+QISEND=1).
// 5. Read HTTP server response from socket buffer (AT+QIRD=1,1500).
// 6. Parse HTTP status code (200 OK, 400, 401, 403, 404, 429, 500) and return success flag.
// 7. Cleanup TCP Socket 1 (closeSocket()).
//
// USED BY:
// - UploadTask (executes sendToUbidots under modemMutex protection)
// ============================================================

// ============================================================
// TRANSMIT TELEMETRY TO UBIDOTS VIA HTTP POST
// ============================================================
// Opens TCP Socket 1, posts JSON telemetry to Ubidots REST API endpoint, parses HTTP status code, and closes socket.
bool sendToUbidots(float liquidLevel, float pressure, int sensorStatus)
{
    Serial.println();
    Serial.println("================================================");
    Serial.println("Sending Data to Ubidots");
    Serial.println("================================================");

    // -------------------------------------------------
    // 1. Build JSON Payload
    // -------------------------------------------------
    String payload = "{\"";
    payload += VARIABLE_LABEL_LEVEL;
    payload += "\":";
    payload += String(liquidLevel, 2);

    payload += ",\"pressure\":";
    payload += String(pressure, 2);

    payload += ",\"";
    payload += VARIABLE_LABEL_STATUS;
    payload += "\":";
    payload += String(sensorStatus);

    payload += "}";

    Serial.println("Payload:");
    Serial.println(payload);

    if (sensorStatus == 0)
    {
        Serial.println("WARNING : Sensor disconnected");
    }

    // -------------------------------------------------
    // 2. Build HTTP Request Header and Body
    // -------------------------------------------------
    String url = "/api/v1.6/devices/";
    url += DEVICE_LABEL;

    String httpRequest = "POST " + url + " HTTP/1.1\r\n";
    httpRequest += "Host: " + String(UBIDOTS_SERVER) + "\r\n";
    httpRequest += "X-Auth-Token: " + String(UBIDOTS_TOKEN) + "\r\n";
    httpRequest += "Content-Type: application/json\r\n";
    httpRequest += "Content-Length: " + String(payload.length()) + "\r\n";
    httpRequest += "Connection: close\r\n";
    httpRequest += "\r\n";
    httpRequest += payload;

    Serial.println();
    Serial.println("================================================");
    Serial.println("HTTP REQUEST");
    Serial.println("================================================");
    Serial.print("Server : ");
    Serial.println(UBIDOTS_SERVER);
    Serial.print("Port   : ");
    Serial.println(UBIDOTS_PORT);
    Serial.print("Device : ");
    Serial.println(DEVICE_LABEL);
    Serial.print("URL    : ");
    Serial.println(url);
    Serial.println("----------------------------------------");
    Serial.println(httpRequest);
    Serial.println("================================================");

    // -------------------------------------------------
    // 3. Close any stale TCP socket on Socket 1
    // -------------------------------------------------
    closeSocket();

    // -------------------------------------------------
    // 4. Open TCP Socket 1
    // -------------------------------------------------
    Serial.println("[STATE] Opening TCP socket 1...");

    String openCommand =
        "AT+QIOPEN=1,1,\"TCP\",\"" +
        String(UBIDOTS_SERVER) +
        "\"," +
        String(UBIDOTS_PORT);

    if (!sendAT(openCommand, "+QIOPEN: 1,0", 10000))
    {
        Serial.println("[STATE] ERROR: Failed to open TCP socket 1");
        return false;
    }

    Serial.println("[STATE] TCP socket 1 open");

    // -------------------------------------------------
    // 5. Request send prompt
    // -------------------------------------------------
    Serial.print("[STATE] Requesting send of ");
    Serial.print(httpRequest.length());
    Serial.println(" bytes on socket 1...");

    EC200.println("AT+QISEND=1," + String(httpRequest.length()));

    if (!waitForSendPrompt(5000))
    {
        Serial.println("[STATE] ERROR: '>' prompt not received - aborting send");
        closeSocket();
        return false;
    }

    Serial.println("[STATE] '>' prompt received, writing HTTP request...");

    // -------------------------------------------------
    // 6. Transmit HTTP request payload
    // -------------------------------------------------
    EC200.print(httpRequest);
    EC200.write(0x1A);

    String sendAck;

    if (!waitForModemResponse("SEND OK", 10000, sendAck))
    {
        Serial.println("[STATE] ERROR: 'SEND OK' not received - upload failed");
        closeSocket();
        return false;
    }

    Serial.println("[STATE] SEND OK received");

    // -------------------------------------------------
    // 7. Wait for server recv URC
    // -------------------------------------------------
    Serial.println("[STATE] Waiting for URC (\"recv\",1) ...");

    String urc;
    bool gotRecvUrc = waitForModemResponse("\"recv\",1", 15000, urc);

    if (!gotRecvUrc)
    {
        Serial.println("[STATE] WARNING: recv URC not seen within timeout");
        Serial.println("[STATE] Attempting to read TCP buffer anyway");
    }
    else
    {
        Serial.println("[STATE] Server response detected");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // -------------------------------------------------
    // 8. Read HTTP Response from TCP Socket Buffer
    // -------------------------------------------------
    Serial.println("[STATE] Issuing AT+QIRD=1,1500 ...");

    while (EC200.available())
    {
        EC200.read();
    }

    EC200.println("AT+QIRD=1,1500");

    String httpResponse;
    readUntilIdle(5000, 400, httpResponse);

    Serial.println();
    Serial.println("[STATE] --- RAW HTTP RESPONSE START ---");
    Serial.print(httpResponse);
    Serial.println("\n[STATE] --- RAW HTTP RESPONSE END ---");

    // -------------------------------------------------
    // 9. Parse HTTP status code from response
    // -------------------------------------------------
    int statusCode = 0;
    int httpIndex = httpResponse.indexOf("HTTP/");

    if (httpIndex >= 0)
    {
        int spaceIndex = httpResponse.indexOf(' ', httpIndex);

        if (spaceIndex >= 0 && httpResponse.length() >= spaceIndex + 4)
        {
            String statusString = httpResponse.substring(spaceIndex + 1, spaceIndex + 4);
            statusCode = statusString.toInt();
        }
    }

    Serial.println();
    Serial.println("================================================");
    Serial.print("[STATE] HTTP status code: ");
    Serial.println(statusCode);

    // -------------------------------------------------
    // 10. Process upload result
    // -------------------------------------------------
    bool uploadSuccess = false;

    switch (statusCode)
    {
        case 200:
        case 201:
        case 202:
        case 204:
            Serial.println("[STATE] Upload SUCCESSFUL");
            uploadCount++;
            uploadSuccess = true;
            break;

        case 400:
            Serial.println("[STATE] ERROR 400: Bad Request");
            break;

        case 401:
            Serial.println("[STATE] ERROR 401: Unauthorized - check UBIDOTS_TOKEN");
            break;

        case 403:
            Serial.println("[STATE] ERROR 403: Forbidden");
            break;

        case 404:
            Serial.println("[STATE] ERROR 404: Not Found - check DEVICE_LABEL/URL");
            break;

        case 429:
            Serial.println("[STATE] ERROR 429: Rate limited");
            break;

        case 500:
            Serial.println("[STATE] ERROR 500: Ubidots server error");
            break;

        case 503:
            Serial.println("[STATE] ERROR 503: Ubidots unavailable");
            break;

        default:
            Serial.println("[STATE] ERROR: No valid HTTP status code found");
            break;
    }

    Serial.println("================================================");

    // -------------------------------------------------
    // 11. Close TCP Socket 1
    // -------------------------------------------------
    closeSocket();

    Serial.print("Upload Count : ");
    Serial.println(uploadCount);
    Serial.println("================================================");

    return uploadSuccess;
}
