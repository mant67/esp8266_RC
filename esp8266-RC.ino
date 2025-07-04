#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <Servo.h>
#include "UdpReceiver.h"

// ===============================
// ⚙️ CONFIGURAZIONE
// ===============================

#define SERVO_X_PIN 2     // GPIO2 -> servo sterzo
#define SERVO_Y_PIN 15    // GPIO15 -> servo acceleratore/freno

const byte DNS_PORT = 53;                    // Porta per DNS captive
IPAddress apIP(192, 168, 4, 1);              // IP fisso dell’ESP in modalità AP
UdpReceiver receiver(4210);                  // Porta UDP (deve combaciare con quella dell'app)
DNSServer dnsServer;

Servo servoX, servoY;                        // Istanze dei servomotori

// ===============================
// 🔁 Funzione di mappatura valori joystick [-100, 100] → angolo servo [0, 180]
// ===============================
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);         // Limita il range per sicurezza
  float normalized = (float)(value + 100) / 200.0f;  // Normalizza tra 0.0 e 1.0
  return (int)(normalized * 180.0f);           // Converte in angolo servo
}

// ===============================
// 🛠️ SETUP iniziale
// ===============================
void setup() {
  Serial.begin(115200);
  delay(100);

  // 🔧 Genera SSID univoco: esempio "ESP_1A2B3C"
  uint32_t chipId = ESP.getChipId();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "ESP_%06X", chipId);

  // 📡 Configura modalità Access Point (senza password)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, nullptr, 1, false, 1);  // 1 client massimo, canale 1, visibile

  // 🖨️ Info di debug sulla rete
  Serial.println();
  Serial.print("Access Point attivo. SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // 🌐 Avvia server DNS per rispondere sempre con l’IP dell’ESP (captive DNS)
  dnsServer.start(DNS_PORT, "*", apIP);

  // ⚙️ Inizializza i servo motori
  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  // 📥 Inizializza ricezione UDP
  receiver.begin();

  // 📦 Callback per ricevere e gestire i comandi
  receiver.onCommand([](const char* msg) {
    Serial.print("Comando ricevuto: ");
    Serial.println(msg);

    int x = 0, y = 0;
    if (sscanf(msg, "X:%d,Y:%d", &x, &y) == 2) {
      int angleX = mapJoystickToServo(x);
      int angleY = mapJoystickToServo(y);

      // Usa x, y per controllare servomotori, ecc.
      Serial.print("X = "); Serial.print(x);
      Serial.print(" | Y = "); Serial.println(y);

      // Evita di inviare valori identici al servo (risparmio energia/disturbi)
      static int lastAngleX = -1, lastAngleY = -1;
      if (angleX != lastAngleX) {
        servoX.write(angleX);
        lastAngleX = angleX;
      }
      if (angleY != lastAngleY) {
        servoY.write(angleY);
        lastAngleY = angleY;
      }
    }
  });
}

// ===============================
// 🔄 LOOP principale
// ===============================
void loop() {
  dnsServer.processNextRequest(); // 🔄 Gestisce le richieste DNS (non blocca)
  receiver.update();              // 🔄 Controlla eventuali pacchetti UDP ricevuti
}
