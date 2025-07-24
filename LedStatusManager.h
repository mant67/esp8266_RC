// LedStatusManager.h
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
  }

  void setStatus(LedStatus newStatus) {
    if (newStatus == currentStatus) return;

    currentStatus = newStatus;
    ledTicker.detach(); // Ferma eventuale lampeggio in corso

    switch (currentStatus) {
      case NO_BINDING:
        ledTicker.attach_ms(100, []() { toggleLed(); }); // Lampeggio veloce
        break;
      case BINDING_NO_CONNECTION:
        ledTicker.attach_ms(500, []() { toggleLed(); }); // Lampeggio lento
        break;
      case CONNECTED:
        ledTicker.detach();
        digitalWrite(CONNECTION_LED_PIN, HIGH); // LED acceso fisso
        break;
    }
  }

  LedStatus getStatus() {
    return currentStatus;
  }

private:
  Ticker ledTicker;
  LedStatus currentStatus = NO_BINDING;
  static bool ledState;

  static void toggleLed() {
    ledState = !ledState;
    digitalWrite(CONNECTION_LED_PIN, ledState);
  }
};

// Definizione statica
bool LedStatusManager::ledState = false;
