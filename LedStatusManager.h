#pragma once
#include <Arduino.h>
#include <Ticker.h>
#include "config.h"

class LedStatusManager {
public:
  enum LedStatus {
    NO_BINDING,
    BINDING_NO_CONNECTION,
    CONNECTED
  };

  void begin() {
    pinMode(CONNECTION_LED_PIN, OUTPUT);
    digitalWrite(CONNECTION_LED_PIN, LOW);
    currentStatus = NO_BINDING;
    instance = this;
    ledTicker.attach_ms(100, toggleLedStatic);
    DEBUG_PRINTLN("🔴 Stato iniziale LED: NO_BINDING (lampeggio veloce)");
  }

  void setStatus(LedStatus newStatus) {
    if (newStatus == currentStatus) return;

    currentStatus = newStatus;
    applyStatus(currentStatus);
  }

  LedStatus getStatus() {
    return currentStatus;
  }

  // 🔄 Ripristina lo stato LED attuale (utile dopo lampeggio temporaneo)
  void refresh() {
    applyStatus(currentStatus);
  }

private:
  Ticker ledTicker;
  LedStatus currentStatus = NO_BINDING;
  bool ledState = false;

  void toggleLed() {
    ledState = !ledState;
    digitalWrite(CONNECTION_LED_PIN, ledState);
  }

  static void toggleLedStatic() {
    if (instance) instance->toggleLed();
  }

  void applyStatus(LedStatus status) {
    ledTicker.detach(); // Ferma qualsiasi lampeggio precedente

    switch (status) {
      case NO_BINDING:
        ledTicker.attach_ms(100, toggleLedStatic); // Lampeggio veloce
        break;
      case BINDING_NO_CONNECTION:
        ledTicker.attach_ms(500, toggleLedStatic); // Lampeggio lento
        break;
      case CONNECTED:
        digitalWrite(CONNECTION_LED_PIN, HIGH); // LED acceso fisso
        ledState = true;
        break;
    }
  }

  static LedStatusManager* instance;
};

// ⚠️ Definizione della variabile statica
LedStatusManager* LedStatusManager::instance = nullptr;
