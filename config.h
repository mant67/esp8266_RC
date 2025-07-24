#pragma once

// ========== ⚙️ Configurazione EEPROM ==========
#define EEPROM_SIZE 64
#define EEPROM_ADDR 0

// ========== 📍 Pinout ==========
#define RESET_PIN 14 
#define CONNECTION_LED_PIN 0
#define SERVO_X_PIN 2
#define SERVO_Y_PIN 15

// ========== 📶 Porta UDP ==========
#define UDP_PORT 4210

// ========== ⏱️ Timeout e Durate ==========
#define CONNECTION_TIMEOUT 6000UL     // ms
#define RESET_HOLD_TIME 3000UL        // ms
#define ACTIVITY_BLINK_DURATION 10UL  // ms

// ========== 🛠️ Debug ==========
#define USE_SERIAL 1
#if USE_SERIAL
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif
