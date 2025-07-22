
#include "ReceiverHandler.h"
#include "config.h"


void setupReceiver(UdpReceiver& receiver, Servo& servoX, Servo& servoY, unsigned long& lastPingTime) {
  receiver.begin();
  receiver.onCommand([&](const char* msg) {
    Serial.print("Comando ricevuto: ");
    Serial.println(msg);
    //DEBUG_PRINT("Comando ricevuto: ");
    //DEBUG_PRINTLN(msg);

    if (strcmp(msg, "PING") == 0) {
      lastPingTime = millis();
      return;
    }

    int x = 0, y = 0;
    if (sscanf(msg, "X:%d,Y:%d", &x, &y) == 2) {
      int angleX = map(x, -100, 100, 0, 180);
      int angleY = map(y, -100, 100, 0, 180);

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
