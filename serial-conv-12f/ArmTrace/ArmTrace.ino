#include <ESP8266WiFi.h>

// Include our network handler files
#include <LocalNetworkSys.h>

IPAddress  serverIP ;
WiFiClient traceClient; // connection to the trace server

void setup() {
    Serial.begin(115200);
    delay(20);

    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);

    // connects to access point
    WiFi.mode(WIFI_STA);
    WiFi.begin(LocalNetworkSys::SSID, LocalNetworkSys::PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        // Serial.print('.');
        delay(100);
    }

    LocalNetworkSys::GetServerAddress gsv(LocalNetworkSys::SERVER_HOST_NAME, LocalNetworkSys::IP_LOOKUP_TCP_PORT, LocalNetworkSys::TRACE_SERVER_NAME) ;
    if (gsv.waitComplete (10000) > 0) {
        serverIP = gsv.ipFound() ;

        int attempts = 100 ;

        while (!traceClient.connect(serverIP, LocalNetworkSys::TRACE_PORT) && (attempts > 0)) {
            delay(100);
            --attempts ;
        }

        if (!traceClient.connected()) {
            // Serial.println("Failed to connect 1");
            // no point carrying on
            ESP.restart() ;
        }
        else {
            // Serial.println("");
            // Serial.println("Connected...");
            // Serial.println("");
            digitalWrite(D4, LOW); // indicate it's connected
        }
    }
    else {
        // no point carrying on
        // Serial.println("Failed to connect 2");
        ESP.restart() ;
    }
}

#define BUFFER_LENGTH 4096
unsigned char  _receiveBuffer[BUFFER_LENGTH] ;
int            _bufferPos  = 0 ;


int   receiveByte  (void) {
    int receivedByte = -1 ;
    if (Serial.available()) {
        int icReturn = Serial.read() ;
        if (icReturn != -1) {
            receivedByte = icReturn ;
        }
        else {
            delay(1) ; // tiny delay to let the buffer fill
        }
    }
    else {
        delay(1) ; // tiny delay to let the buffer fill
    }

    return receivedByte ;
}

void loop() {
    int  dataIn = receiveByte () ;

    if (dataIn >= 0) {
        if (dataIn == '\n') {
            // packet complete
            _receiveBuffer[_bufferPos++] = '\r' ;

            if (!traceClient.connected()) {
                traceClient.connect(serverIP, LocalNetworkSys::TRACE_PORT) ;
            }

            if (traceClient.connected()) {
                // send and clear down the buffer
                traceClient.write (_receiveBuffer, _bufferPos) ;
                _bufferPos = 0 ;
            }
        }
        else {
            if (dataIn >= 0x20 && dataIn < 256) { // Don't store junk characters
                _receiveBuffer[_bufferPos++] = (unsigned char) dataIn ;
            }
        }
    }
}


