
#pragma once
#include <Servo.h>
#include "UdpReceiver.h"

void setupReceiver(UdpReceiver& receiver, Servo& servoX, Servo& servoY, unsigned long& lastPingTime);
