#include <ESP8266WiFi.h>

// Include our network handler files
#include <LocalNetworkSys.h>

IPAddress  traceServerIP ;
IPAddress  applicationServerIP ;
WiFiClient traceClient   ; // connection to the trace server
WiFiClient appClient     ; // connection to the application server

unsigned long nextTime ;

void setup() {
    Serial.begin(115200);
    delay(20);
    Serial.println("Traceclient type 2") ;

    // connects to access point
    WiFi.mode(WIFI_STA);
    WiFi.begin(LocalNetworkSys::SSID, LocalNetworkSys::PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }

    // Get the IP address of the trace server
    bool complete = false ;

    while (!complete) {
        LocalNetworkSys::GetServerAddress gsv1(LocalNetworkSys::SERVER_HOST_NAME, LocalNetworkSys::IP_LOOKUP_TCP_PORT, LocalNetworkSys::TRACE_SERVER_NAME) ;
        if (gsv1.waitComplete (10000) > 0) {
            traceServerIP = gsv1.ipFound() ;
            Serial.printf("Trace server IP       = %s\r\n", traceServerIP.toString().c_str());
            complete = true ;
        }
        else {
            Serial.println("Error getting trace server IP");
            delay(5000) ;
        }
    }

    complete = false ;

    while (!complete) {
        // Get the IP address of the application server Called "Y_MOTOR_OUT"
        LocalNetworkSys::GetServerAddress gsv2(LocalNetworkSys::SERVER_HOST_NAME, LocalNetworkSys::IP_LOOKUP_TCP_PORT, "Y_MOTOR_OUT") ;
        if (gsv2.waitComplete (10000) > 0) {
            applicationServerIP = gsv2.ipFound() ;
            Serial.printf("Application server %s  IP = %s\r\n", "Y_MOTOR_OUT", applicationServerIP.toString().c_str());
            complete = true ;
        }
        else {
            Serial.println("Error getting application server IP");
            delay(5000) ;
        }
    }
    Serial.println("") ;

    bool ready = false ;
    while (!ready) {
        if (!traceClient.connected()) {
            traceClient.connect(traceServerIP, LocalNetworkSys::TRACE_PORT) ;
        }

        if (!appClient.connected()) {
            appClient.connect(applicationServerIP, 7999) ;
        }

        if (traceClient.connected() && appClient.connected()) {
            Serial.println("Connected to both servers");
            ready = true ; // all done
        }
        else {
            Serial.println("Failed to connect to both servers") ;
            delay(5000) ;
        }
    }

    nextTime = 0 ;
}

void loop() {
    if (millis() > nextTime) {
        if (!traceClient.connected()) {
            traceClient.connect(traceServerIP, LocalNetworkSys::TRACE_PORT) ;
        }

        if (traceClient.connected()) {
            nextTime = millis() + 12000 ; // wait 12 seconds before next attempt
            unsigned char buffer[35] ;

            for (int i = 0; i < sizeof(buffer); i++) {
                buffer[i] = i ;
            }

            Serial.println("Writing buffer to trace server") ;
            traceClient.write (buffer, sizeof(buffer)) ;

            // Send the server data
            if (!appClient.connected()) {
                appClient.connect(applicationServerIP, 7999) ;
            }

            if (appClient.connected()) {
                char buffer2[]={"This is an application message\r\n"} ;

                Serial.println("Writing string to application client") ;
                appClient.write (buffer2, strlen(buffer2)) ;
            }
        }
    }
}


