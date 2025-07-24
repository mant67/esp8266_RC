#include "config.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Servo.h>
#include "ReceiverHandler.h"
#include "UdpReceiver.h"
#include "MacManager.h"
#include "WiFiManager.h"
#include "LedStatusManager.h"

UdpReceiver receiver(4210);
Servo servoX, servoY;
LedStatusManager ledManager;

volatile bool resetPending = false; 

unsigned long lastPingTime = 0;
const unsigned long CONNECTION_TIMEOUT = 6000;

volatile unsigned long resetStartTime = 0;
const unsigned long RESET_HOLD_TIME = 3000;

// ===============================
// 🎮 Mappatura joystick → servo
// ===============================
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);
  float normalized = (float)(value + 100) / 200.0f;
  return (int)(normalized * 180.0f);
}

// ===============================
// 🔁 Interrupt: pressione pulsante (falling)
// ===============================
void ICACHE_RAM_ATTR onResetPressed() {
  if (digitalRead(RESET_PIN) == LOW) {
    resetStartTime = millis();
  }
}

// ===============================
// 🔁 Interrupt: rilascio pulsante (rising)
// ===============================
void ICACHE_RAM_ATTR onResetReleased() {
  unsigned long duration = millis() - resetStartTime;
  if (duration >= RESET_HOLD_TIME) {
    resetPending = true;   // ⚠️ da gestire nel loop
    DEBUG_PRINTLN("✅ MAC cancellato. Reset logico in arrivo...");
  } else {
    DEBUG_PRINTLN("❌ Pulsante rilasciato troppo presto. Reset annullato.");
  }
  resetStartTime = millis();
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

  ledManager.begin();

  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  String mac = readMacFromEEPROM();
  if (isValidMac(mac)) {
    DEBUG_PRINT("📂 MAC caricato da EEPROM: ");
    DEBUG_PRINTLN(mac);
    ledManager.setStatus(LedStatusManager::BINDING_NO_CONNECTION);
  } else {
    DEBUG_PRINTLN("ℹ️ Nessun MAC salvato o memoria sporca.");
    ledManager.setStatus(LedStatusManager::NO_BINDING);
  }

  setupReceiver(receiver, servoX, servoY, lastPingTime);
}

void loop() {
  updateWiFi();
  receiver.update();


  if (resetPending) {
    resetPending = false;

    // 1. Disconnette tutti i client e ferma AP
    WiFi.softAPdisconnect(true);
    delay(100);

    // 2. Pulisci variabili/mac (già fatto nella ISR col clearMacFromEEPROM)
    clearMacFromEEPROM();  // EEPROM reset
    
    // 3. Riavvia Access Point
    setupWiFi();

    // 4. Reinizializza il receiver e led
    // setupReceiver(receiver, servoX, servoY, lastPingTime);
    //ledManager.setStatus(LedStatusManager::NO_BINDING);

    DEBUG_PRINTLN("🔄 Reinizializzazione completa senza reset hardware.");
    ESP.restart(); // ✅ riavvio completo (senza pulsante)
  }

  // Stato connessione via ping
  if (millis() - lastPingTime <= CONNECTION_TIMEOUT) {
    if (ledManager.getStatus() != LedStatusManager::CONNECTED) {
      ledManager.setStatus(LedStatusManager::CONNECTED);
    }
  } else {
    if (ledManager.getStatus() == LedStatusManager::CONNECTED) {
      ledManager.setStatus(LedStatusManager::BINDING_NO_CONNECTION);
    }
  }
}
