#include "WiFiManager.h"
#include "MacManager.h"
#include "config.h"
#include <DNSServer.h>

extern "C" {
  #include "user_interface.h"
  void wifi_softap_deauth(uint8* mac);
}

IPAddress apIP(192,168,4,1);
DNSServer dnsServer;

bool isConnected = false;
unsigned long* pingPtr = nullptr;

String macToString(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void wifiEventHandler(System_Event_t* evt) {
  switch (evt->event) {
    case EVENT_SOFTAPMODE_STACONNECTED: {
      String incomingMac = macToString(evt->event_info.sta_connected.mac);
      DEBUG_PRINT("Connesso: "); DEBUG_PRINTLN(incomingMac);
      digitalWrite(CONNECTION_LED_PIN, HIGH);
      static String storedMac = ""; //readMacFromEEPROM();

      if (storedMac.length() == 0) {
        //saveMacToEEPROM(incomingMac);
        storedMac = incomingMac;
        DEBUG_PRINTLN("✅ Dispositivo associato.");
      } else if (incomingMac != storedMac) {
        DEBUG_PRINTLN("⛔ MAC non autorizzato!");
        wifi_softap_deauth((uint8*)evt->event_info.sta_connected.mac);
        return;
      }

      isConnected = true;
      break;
    }
    case EVENT_SOFTAPMODE_STADISCONNECTED: {
      DEBUG_PRINT("Disconnesso: ");
      DEBUG_PRINTLN(macToString(evt->event_info.sta_disconnected.mac));
      digitalWrite(CONNECTION_LED_PIN, LOW);
      isConnected = false;
      break;
    }
  }
}

void setupWiFi() {
  pinMode(CONNECTION_LED_PIN, OUTPUT);
  digitalWrite(CONNECTION_LED_PIN, LOW);

  wifi_set_event_handler_cb(wifiEventHandler);
  WiFi.mode(WIFI_AP);

  uint32_t chipId = ESP.getChipId();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "ESP_%06X", chipId);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, "ESP12345", 1, false, 1);

  dnsServer.start(53, "*", apIP);

  DEBUG_PRINTLN();
  DEBUG_PRINT("AP attivo: "); DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP: "); DEBUG_PRINTLN(WiFi.softAPIP());
}

void updateWiFi() {
  dnsServer.processNextRequest();  // fondamentale!
}
