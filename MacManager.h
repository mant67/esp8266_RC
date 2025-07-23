#pragma once
#include <Arduino.h>

void saveMacToEEPROM(const String& mac);
String readMacFromEEPROM();
void clearMacFromEEPROM();
bool isValidMac(const String& mac);
