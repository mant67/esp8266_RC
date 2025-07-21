#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <Servo.h>
#include "UdpReceiver.h"
#include <EEPROM.h>

extern "C" {
  #include "user_interface.h"
}

// ===============================
// ⚙️ CONFIGURAZIONE
// ===============================

#define EEPROM_SIZE 64
#define EEPROM_ADDR 0

#define CONNECTION_LED_PIN 0
#define SERVO_X_PIN 2
#define SERVO_Y_PIN 15

unsigned long lastPingTime = 0;
const unsigned long CONNECTION_TIMEOUT = 6000;
bool isConnected = false;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
UdpReceiver receiver(4210);
DNSServer dnsServer;

Servo servoX, servoY;

// ===============================
// 🛠️ FUNZIONI DI GESTIONE LOW-LEVEL
// ===============================
extern "C" {
#include "user_interface.h"
void wifi_softap_deauth(uint8 *mac);
}


String macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void wifiEventHandler(System_Event_t* evt) {
    switch (evt->event) {
        case EVENT_SOFTAPMODE_STACONNECTED: {
            // Aggiungi parentesi per creare un nuovo scope
            String incomingMac = macToString(evt->event_info.sta_connected.mac);
            Serial.print("Connesso: ");
            Serial.println(incomingMac);
            digitalWrite(CONNECTION_LED_PIN, HIGH);

            static String storedMac = readMacFromEEPROM();
            if (storedMac.length() == 0) {
                saveMacToEEPROM(incomingMac);
                storedMac = incomingMac;
                Serial.println("✅ Dispositivo associato.");
                lastPingTime = millis();
                isConnected = true;
            } else if (incomingMac != storedMac) {
                Serial.println("⛔ MAC non autorizzato! Connessione non accettata.");
                 wifi_softap_deauth((uint8*)evt->event_info.sta_connected.mac);  // ✅ CORRETTO
            } else {
                Serial.println("🔐 Connessione accettata.");
                lastPingTime = millis();
                isConnected = true;
            }
            break;
        }
            
        case EVENT_SOFTAPMODE_STADISCONNECTED: {
            // Correggi qui: usa sta_disconnected invece di sta_connected
            uint8_t* mac = evt->event_info.sta_disconnected.mac;
            Serial.print("Disconnesso: ");
            Serial.println(macToString(mac));
            digitalWrite(CONNECTION_LED_PIN, LOW);
            break;
        }
    }
}




// ===============================
// 🔁 Mapping joystick → servo
// ===============================
int mapJoystickToServo(int value) {
  value = constrain(value, -100, 100);
  float normalized = (float)(value + 100) / 200.0f;
  return (int)(normalized * 180.0f);
}

// ===============================
// 🔁 salva MAC nella EEPROM
// ===============================
void saveMacToEEPROM(const String& mac) {
  for (int i = 0; i < mac.length(); i++) {
    EEPROM.write(EEPROM_ADDR + i, mac[i]);
  }
  EEPROM.write(EEPROM_ADDR + mac.length(), '\0');
  EEPROM.commit();
  Serial.print("💾 MAC salvato in EEPROM: ");
  Serial.println(mac);
}

// ===============================
// 🔁 legge MAC dalla EEPROM
// ===============================
String readMacFromEEPROM() {
  char mac[32];
  int i = 0;
  while (i < sizeof(mac) - 1) {
    char c = EEPROM.read(EEPROM_ADDR + i);
    if (c == '\0') break;
    // Se il carattere non è stampabile (ASCII tra 32 e 126), consideriamo la EEPROM "sporca"
    if (c < 32 || c > 126) {
      Serial.println("⚠️ EEPROM contiene caratteri non validi. Considerata vuota.");
      return "";
    }
    mac[i++] = c;
  }
  mac[i] = '\0';

  if (i == 0) {
    return "";
  }

  return String(mac);
}


// ===============================
// 🔍 Verifica se il MAC salvato è valido (evita dati sporchi)
// ===============================
bool isValidMac(const String& mac) {
  return mac.length() == 17 && mac.indexOf(':') == 2; // formato tipico "XX:XX:XX:XX:XX:XX"
}


// ===============================
// 🛠️ SETUP iniziale
// ===============================
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(CONNECTION_LED_PIN, OUTPUT);
  digitalWrite(CONNECTION_LED_PIN, LOW);

  // Registra l'handler di sistema LOW-LEVEL
  wifi_set_event_handler_cb(wifiEventHandler);

  uint32_t chipId = ESP.getChipId();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "ESP_%06X", chipId);

  WiFi.mode(WIFI_AP);

  const char* password = "ESP12345";  // password
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password, 1, false, 1);
 

  Serial.println();
  Serial.print("Access Point attivo. SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", apIP);

  servoX.attach(SERVO_X_PIN);
  servoY.attach(SERVO_Y_PIN);

  EEPROM.begin(EEPROM_SIZE);

  // 🔁 Carica il MAC salvato
  String storedMac = readMacFromEEPROM();
  if (isValidMac(storedMac)) {
    Serial.print("📂 MAC caricato da EEPROM: ");
    Serial.println(storedMac);
  } else {
    Serial.println("ℹ️ Nessun MAC salvato o memoria sporca.");
    storedMac = ""; // azzera se non valido
  }


  receiver.begin();
  receiver.onCommand([](const char* msg) {
    Serial.print("Comando ricevuto: ");
    Serial.println(msg);

    if (strcmp(msg, "PING") == 0) {
      lastPingTime = millis();
      return;
    }

    int x = 0, y = 0;
    if (sscanf(msg, "X:%d,Y:%d", &x, &y) == 2) {
      int angleX = mapJoystickToServo(x);
      int angleY = mapJoystickToServo(y);

      Serial.print("X = "); Serial.print(x);
      Serial.print(" | Y = "); Serial.println(y);

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
// 🔁 LOOP principale
// ===============================

unsigned long lastClientCheck = 0;
const unsigned long CLIENT_CHECK_INTERVAL = 2000;

void loop() {
  dnsServer.processNextRequest();
  receiver.update();

  if (isConnected && millis() - lastPingTime > CONNECTION_TIMEOUT) {
    isConnected = false;
    Serial.println("❌ Timeout connessione. LED spento.");
    digitalWrite(CONNECTION_LED_PIN, LOW);
  }

}
