#include "config.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Servo.h>
#include "ReceiverHandler.h"
#include "UdpReceiver.h"
#include "MacManager.h"
#include "WiFiManager.h"

UdpReceiver receiver(4210);

Servo servoX, servoY;

unsigned long lastPingTime = 0;
const unsigned long CONNECTION_TIMEOUT = 6000;

// ===============================
// 🔁 Mapping joystick → servo
// ===============================
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);
  float normalized = (float)(value + 100) / 200.0f;
  return (int)(normalized * 180.0f);
}

void setup() {
  #if USE_SERIAL
    Serial.begin(115200);
  #endif

  EEPROM.begin(EEPROM_SIZE);
  setupWiFi();

  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  String mac = readMacFromEEPROM();
  if (isValidMac(mac)) {
    DEBUG_PRINT("📂 MAC caricato da EEPROM: ");
    DEBUG_PRINTLN(mac);
  } else {
    DEBUG_PRINTLN("ℹ️ Nessun MAC salvato o memoria sporca.");
  }

  setupReceiver(receiver, servoX, servoY, lastPingTime);
}

void loop() {
  updateWiFi();
  receiver.update();

  if (millis() - lastPingTime > CONNECTION_TIMEOUT) {
    digitalWrite(CONNECTION_LED_PIN, LOW);
  }
}
