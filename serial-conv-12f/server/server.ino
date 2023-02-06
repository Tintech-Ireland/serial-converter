#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
// #include <ESP8266mDNS.h>

// Include our network handler files
#include <LocalNetworkSys.h>

#define SERIAL_OUTPUT 0

// if you want to see the serial port stuff - set this to 1
// #define SERIAL_OUTPUT 1

namespace LocalNetworkSys {

class MotorServer : public SystemServer {
public:
    MotorServer (const char * host, int port, int serverPort, const char * serverName) : SystemServer (host, port, serverPort, serverName) {}
    virtual ~MotorServer() {}

protected:

    // Just provide this function to deal with data requests
    // YOU CAN reply but be careful
    virtual void response(AsyncClient* client, void *data, unsigned int len) override {
        // Serial.printf("Received data from client\r\n");
        char reply[32];
        sprintf(reply, "this is from %s", client->remoteIP().toString().c_str());

        trace (reply) ;                        // Trace a dumb message
        trace ((unsigned char *) data, len) ;  // trace the data we received

        #if SERIAL_OUTPUT
            unsigned char * p = (unsigned char *) data ;
            for (int i = 0; i < len; i++) {
                Serial.printf("%02X ", p[i]);
            }
            Serial.println("") ;
        #endif
    }

} ;

} // namespace LocalNetworkSys

LocalNetworkSys::MotorServer * localServer = NULL ;

void setup() {
    #if SERIAL_OUTPUT
        Serial.begin(115200);
        delay(20);
        Serial.println("Server") ;
    #endif

    // connects to access point
    WiFi.mode(WIFI_STA);
    WiFi.begin(LocalNetworkSys::SSID, LocalNetworkSys::PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        // Serial.print('.');
        delay(500);
    }

    // just create the server - This does everything - you HAVE to do this after the
    // Wifi system is up and running
    localServer = new LocalNetworkSys::MotorServer (LocalNetworkSys::SERVER_HOST_NAME,
                                                    LocalNetworkSys::IP_LOOKUP_TCP_PORT,
                                                    7999,  // This is the port WE run on
                                                    "Y_MOTOR_OUT") ;
}

void loop() {
    // You don't really need to put anyrthing in here - it all runs asynchronously
}


