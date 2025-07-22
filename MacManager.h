#pragma once
#include <Arduino.h>

void saveMacToEEPROM(const String& mac);
String readMacFromEEPROM();
bool isValidMac(const String& mac);
