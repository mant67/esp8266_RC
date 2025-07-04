/*
 * utilizzo:

 #include "BatteryMonitor.h"
 Impostazioni partitore resistivo per batteria 7.4V
 const float R1 = 100000.0;  // 100k ohm
 const float R2 = 15000.0;   // 15k ohm
 BatteryMonitor battMon(A0, R1, R2, 1.0, 1023);

void setup() {
  // Init battery monitor
  battMon.begin();
}

void loop() {
  loat voltage = battMon.readBatteryVoltage();
  Serial.print("Tensione batteria: ");
  Serial.print(voltage);
  Serial.println(" V");
  delay(1000);
}
 */

#ifndef BatteryMonitor_h
#define BatteryMonitor_h

#include <Arduino.h>  // Necessario per analogRead, uint8_t, ecc.

class BatteryMonitor {
  private:
    uint8_t adcPin;
    float R1, R2;         // Resistenze del partitore
    float adcMaxVoltage;  // Max tensione sul pin ADC (es. 1.0V o 3.3V a seconda del modello)
    int adcResolution;    // Risoluzione ADC (tipicamente 1023 per 10 bit)

  public:
    BatteryMonitor(uint8_t pin, float r1, float r2, float maxVoltage = 1.0, int resolution = 1023)
      : adcPin(pin), R1(r1), R2(r2), adcMaxVoltage(maxVoltage), adcResolution(resolution) {}

    void begin() {
      // Eventuale setup del pin
    }

    float readBatteryVoltage() {
      int adcValue = analogRead(adcPin);
      float voltageOnPin = (adcValue / (float)adcResolution) * adcMaxVoltage;
      float batteryVoltage = voltageOnPin * ((R1 + R2) / R2);
      return batteryVoltage;
    }
};

#endif  // BatteryMonitor_h
