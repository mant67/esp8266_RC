#ifndef UDP_RECEIVER_H
#define UDP_RECEIVER_H

#include <WiFiUdp.h>

typedef std::function<void(const char* message)> UdpCommandCallback;

class UdpReceiver {
private:
    WiFiUDP udp;
    unsigned int port;
    char packetBuffer[256];
    UdpCommandCallback callback;
    bool packetReceived = false;

public:
    UdpReceiver(unsigned int listenPort = 4210) : port(listenPort) {}

    void begin() {
        udp.begin(port);
        Serial.print("UDP listening on port ");
        Serial.println(port);
    }

    void onCommand(UdpCommandCallback cb) {
        callback = cb;
    }

    void update() {
        int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
            if (len > 0) {
                packetBuffer[len] = '\0'; // Null-terminate
                if (callback) {
                    callback(packetBuffer);
                }
                packetReceived = true;
            }
        }
    }

    bool wasPacketReceived() {
        if (packetReceived) {
            packetReceived = false;
            return true;
        }
        return false;
    }
};

#endif
