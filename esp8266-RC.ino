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

// === Istanze globali ===
UdpReceiver receiver(UDP_PORT);
Servo servoX, servoY;
LedStatusManager ledManager;

// === Stato globale ===
static bool hasBinding = false;
volatile bool resetPending = false;
volatile unsigned long resetStartTime = 0;
unsigned long lastPingTime = 0;
unsigned long lastActivityBlinkTime = 0;
bool activityLedOn = false;

// === Binding accessibile globalmente ===
bool getHasBinding() { return hasBinding; }
void setHasBinding(bool value) { hasBinding = value; }

// === Mappatura joystick → servo (da -100..100 a 0..180) ===
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);
  return (int)(((value + 100) / 200.0f) * 180.0f);
}

// === Gestione pulsante RESET ===
void ICACHE_RAM_ATTR onResetPressed() {
  if (digitalRead(RESET_PIN) == LOW) {
    resetStartTime = millis();
  }
}

void ICACHE_RAM_ATTR onResetReleased() {
  unsigned long duration = millis() - resetStartTime;
  if (duration >= RESET_HOLD_TIME) {
    resetPending = true;
    DEBUG_PRINTLN("✅ MAC cancellato. Reset logico in arrivo...");
  } else {
    DEBUG_PRINTLN("❌ Pulsante rilasciato troppo presto. Reset annullato.");
  }
}

// === Setup iniziale ===
void setup() {
  #if USE_SERIAL
    Serial.begin(115200);
  #endif

  EEPROM.begin(EEPROM_SIZE);
  setupWiFi();

  // Pulsante di reset
  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), onResetPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), onResetReleased, RISING);

  // LED di stato
  ledManager.begin();

  // Servo
  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  // Carica MAC da EEPROM
  String mac = readMacFromEEPROM();
  if (isValidMac(mac)) {
    DEBUG_PRINT("\xF0\x9F\x93\x82 MAC caricato da EEPROM: ");
    DEBUG_PRINTLN(mac);
    hasBinding = true;
    ledManager.setStatus(LedStatusManager::BINDING_NO_CONNECTION);
  } else {
    DEBUG_PRINTLN("\xE2\x84\xB9 Nessun MAC salvato o memoria corrotta.");
    hasBinding = false;
    ledManager.setStatus(LedStatusManager::NO_BINDING);
  }

  setupReceiver(receiver, servoX, servoY, lastPingTime);
  lastPingTime = millis() - CONNECTION_TIMEOUT - 1; // forza primo controllo
}

// === Loop principale ===
void loop() {
  updateWiFi();
  receiver.update();

  // 🔁 Breve blink LED quando si riceve un pacchetto
  if (receiver.wasPacketReceived()) {
    digitalWrite(CONNECTION_LED_PIN, LOW);  // LED spento per attività
    lastActivityBlinkTime = millis();
    activityLedOn = true;
  }

  // 🔁 Dopo il blink, ripristina stato LED
  if (activityLedOn && millis() - lastActivityBlinkTime > ACTIVITY_BLINK_DURATION) {
    activityLedOn = false;
    ledManager.refresh(); // ripristina ON o lampeggio
  }

  // 🔁 Reset logico se richiesto da pulsante
  if (resetPending) {
    resetPending = false;
    WiFi.softAPdisconnect(true);
    delay(100);
    clearMacFromEEPROM();
    hasBinding = false;
    setupWiFi();
    DEBUG_PRINTLN("♻️ Reinizializzazione senza reset hardware.");
    ESP.restart();
  }

  // 🔁 Aggiorna stato LED in base alla connessione (binding attivo)
  if (hasBinding) {
    LedStatusManager::LedStatus desiredStatus = 
      (millis() - lastPingTime <= CONNECTION_TIMEOUT)
        ? LedStatusManager::CONNECTED
        : LedStatusManager::BINDING_NO_CONNECTION;

    if (ledManager.getStatus() != desiredStatus) {
      DEBUG_PRINTLN(desiredStatus == LedStatusManager::CONNECTED
        ? "\xF0\x9F\x94\xB5 LED: CONNECTED"
        : "\xF0\x9F\x94\xB5 LED: BINDING_NO_CONNECTION");
      ledManager.setStatus(desiredStatus);
    }
  }
}
