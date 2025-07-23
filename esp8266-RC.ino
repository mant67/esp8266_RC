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

volatile unsigned long resetStartTime = 0;
const unsigned long RESET_HOLD_TIME = 3000; // 3 secondi

// ===============================
// 🔁 Mapping joystick → servo
// ===============================
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);
  float normalized = (float)(value + 100) / 200.0f;
  return (int)(normalized * 180.0f);
}



// ===============================
// 🔁 Reset- FALLING
// ===============================
void ICACHE_RAM_ATTR onResetPressed() {
  if (digitalRead(RESET_PIN) == LOW) {
    resetStartTime = millis();
  }
}


// ===============================
// 🔁 Reset- RISING
// ===============================
void ICACHE_RAM_ATTR onResetReleased() {
  unsigned long duration = millis() - resetStartTime;
  if (duration >= RESET_HOLD_TIME) {
    clearMacFromEEPROM();  // EEPROM reset
    DEBUG_PRINTLN("✅ MAC cancellato dalla EEPROM (reset via pulsante).");
  } else {
    DEBUG_PRINTLN("❌ Pulsante rilasciato troppo presto. Reset annullato.");
  }
}




void setup() {
  #if USE_SERIAL
    Serial.begin(115200);
  #endif

  EEPROM.begin(EEPROM_SIZE);
  setupWiFi();


  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), onResetPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), onResetReleased, RISING);
  
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
