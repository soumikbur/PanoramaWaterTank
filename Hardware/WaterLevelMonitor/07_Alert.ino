// ============================================================
// 07_Alert.ino
// ============================================================
// PURPOSE:
// Audible Buzzer and Visual RGB LED Alert Peripheral Driver Tab.
//
// RESPONSIBILITIES:
// 1. Initialize ESP32 LEDC PWM peripheral for piezo buzzer control (Channel 0, 2000 Hz, 8-bit).
// 2. Initialize GPIO pin 39 for status RGB LED control.
// 3. Play tone frequencies, durations, and volume levels.
// 4. Output RGB LED color states (0 = OFF, 1 = ON/Red, 4 = Flashing Red).
//
// HARDWARE CONNECTIONS:
// - Buzzer Pin: GPIO 21
// - RGB LED Pin: GPIO 39
//
// USED BY:
// - AlertTask (drives audible and visual alert patterns based on water level)
// ============================================================

// ============================================================
// INITIALIZE BUZZER AND RGB LED PERIPHERALS
// ============================================================
// Configures GPIO direction and attaches ESP32 LEDC PWM timer to buzzer output.
void initBuzzerAndLED() {
  pinMode(BUZZER, OUTPUT);
  pinMode(RGB_LED, OUTPUT);

  // Attach LEDC channel 0 to buzzer pin with 2000 Hz frequency and 8-bit resolution.
  ledcAttach(BUZZER, BUZZER_FREQ, BUZZER_RESOLUTION);

  // Silence buzzer on startup.
  ledcWrite(BUZZER, 0);

  // Turn OFF RGB LED on startup.
  digitalWrite(RGB_LED, LOW);
}

// ============================================================
// PLAY AUDIBLE TONE ON BUZZER
// ============================================================
// Plays tone at requested frequency (Hz), duration (ms), and volume duty cycle (0..255).
void playTone(int frequency, int duration, int volume) {
  if (frequency > 0) {
    ledcWriteTone(BUZZER, frequency);
    ledcWrite(BUZZER, volume);
  } else {
    ledcWrite(BUZZER, 0);
  }

  // Hold tone for specified duration.
  delay(duration);

  // Silence buzzer after tone duration completes.
  ledcWrite(BUZZER, 0);
}

// ============================================================
// SET RGB LED INDICATOR COLOR / PATTERN
// ============================================================
// Controls RGB LED visual indicator:
//   0 = OFF (LOW)
//   1 = SOLID RED (HIGH)
//   4 = FLASHING RED (Toggle state every 200 ms)
void setRGBColor(int color) {
  if (color == 0) {
    digitalWrite(RGB_LED, LOW);
  } else if (color == 1) {
    digitalWrite(RGB_LED, HIGH);
  } else if (color == 4) {
    static unsigned long lastBlink = 0;
    static bool blinkState = false;

    if (millis() - lastBlink > 200) {
      blinkState = !blinkState;
      digitalWrite(RGB_LED, blinkState ? HIGH : LOW);
      lastBlink = millis();
    }
  }
}
