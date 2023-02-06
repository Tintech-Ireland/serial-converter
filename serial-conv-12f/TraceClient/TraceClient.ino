#include <ESP8266WiFi.h>

// Include our network handler files
#include <LocalNetworkSys.h>

// extern "C" {
// #include <osapi.h>
// #include <os_type.h>
// }

// static os_timer_t intervalTimer;
// static void talkToServer(void* arg) {
//     Serial.printf("Server IP = %s\r\n", serverIP.toString().c_str());
//
//     os_timer_arm(&intervalTimer, 8000, true); // schedule for reply to server at next 8s
// }

IPAddress  serverIP ;
WiFiClient traceClient; // connection to the trace server
unsigned long nextTime ;

void setup() {
    Serial.begin(115200);
    delay(20);
    // Serial.println("Traceclient type 1") ;

    // connects to access point
    WiFi.mode(WIFI_STA);
    WiFi.begin(LocalNetworkSys::SSID, LocalNetworkSys::PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        // Serial.print('.');
        delay(500);
    }

    LocalNetworkSys::GetServerAddress gsv(LocalNetworkSys::SERVER_HOST_NAME, LocalNetworkSys::IP_LOOKUP_TCP_PORT, LocalNetworkSys::TRACE_SERVER_NAME) ;
    if (gsv.waitComplete (10000)) {
        // Serial.printf("Server IP = %s\r\n", gsv.ipFound().toString().c_str());
        serverIP = gsv.ipFound() ;
    }

    // os_timer_disarm(&intervalTimer);
    // os_timer_setfn(&intervalTimer, &talkToServer, NULL);
    // os_timer_arm(&intervalTimer, 8000, true); // schedule for reply to server at next 8s

    if (!traceClient.connect(serverIP, LocalNetworkSys::TRACE_PORT)) {
        delay(5000);
    }

    if (!traceClient.connected()) {
        Serial.println("Failed to connect to server") ;
    }

    nextTime = 0 ;
}

void loop() {
    // Use WiFiClient class to create TCP connections
    if (millis() > nextTime) {
        if (!traceClient.connected()) {
            traceClient.connect(serverIP, LocalNetworkSys::TRACE_PORT) ;
        }

        if (traceClient.connected()) {
            nextTime = millis() + 8000 ; // wait 8 seconds before next attempt
            unsigned char buffer[128] ;

            for (int i = 0; i < sizeof(buffer); i++) {
                buffer[i] = i ;
            }

            traceClient.write (buffer, sizeof(buffer)) ;
        }
    }
}


