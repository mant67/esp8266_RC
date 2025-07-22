#include "MacManager.h"
#include <EEPROM.h>
#include "config.h"

void saveMacToEEPROM(const String& mac) {
  for (int i = 0; i < mac.length(); i++) {
    EEPROM.write(EEPROM_ADDR + i, mac[i]);
  }
  EEPROM.write(EEPROM_ADDR + mac.length(), '\0');
  EEPROM.commit();
  DEBUG_PRINT("💾 MAC salvato in EEPROM: ");
  DEBUG_PRINTLN(mac);
}

String readMacFromEEPROM() {
  char mac[32];
  int i = 0;
  while (i < sizeof(mac) - 1) {
    char c = EEPROM.read(EEPROM_ADDR + i);
    if (c == '\0') break;
    if (c < 32 || c > 126) {
      DEBUG_PRINTLN("⚠️ EEPROM contiene caratteri non validi. Considerata vuota.");
      return "";
    }
    mac[i++] = c;
  }
  mac[i] = '\0';
  return String(mac);
}

bool isValidMac(const String& mac) {
  return mac.length() == 17 && mac.indexOf(':') == 2;
}
