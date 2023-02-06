#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <vector>
#include <map>

// Include our network handler files
#include <LocalNetworkSys.h>

static DNSServer DNS;

/***********************************************************************/
/***********************************************************************/
//                      Trace Services Start
/***********************************************************************/
/***********************************************************************/
static void _ByteToAsciiHex(unsigned char *    OutputString,
                            int                OutputLength,
                            unsigned char      ByteToTranslate) {
    unsigned char hex_digit;

    if (OutputLength > 2) {
        OutputLength = 2;
    }

    while (OutputLength--) {
        hex_digit = (unsigned char)((ByteToTranslate >> 4) | '0');

        if ('9' < hex_digit) {
            hex_digit += (unsigned char)('A' - ('9' + 1));
        }
        *OutputString++ = hex_digit;
        ByteToTranslate <<= 4;
    }
}

static void _POINTER2ASCII(void *           value,
                           char           * ascii_buf,
                           int              len_buf) {
    unsigned char *  cheat = (unsigned char *)&value;
    int         i;
    char     *  current;

    if (ascii_buf) {
        if (len_buf > sizeof(value) * 2) {
            memset (ascii_buf, 0, len_buf) ;
            current = ascii_buf + (sizeof(value) * 2) - 2;
            for (i = 0; i < (int) sizeof(value); i++) {
                _ByteToAsciiHex((unsigned char *)current, 2, cheat[i]);
                --current;
                --current;
            }
        }
    }
}


static void dump_data(const char *          from,
                      const void *          displayBuffer,
                      size_t                out_len,
                      int                   indent) {

    char                 LineBuffer[170];
    int                  BytesRemaining  = out_len;
    int                  ToPrint;
    int                  i;
    int                  LineOffset;
    char                 TempBuffer[60];
    unsigned char     *  DataPtr = (unsigned char *)displayBuffer;
    unsigned char     *  TempPtr;

    if (indent > 20) {
        indent = 20;
    }

    Serial.printf("Data received From IP %s\r\n", from) ;
    while (BytesRemaining) {
        LineOffset = indent;

        ToPrint = std::min(BytesRemaining, 16);
        memset(LineBuffer, ' ', 80 + indent);

        _POINTER2ASCII(DataPtr, TempBuffer, sizeof(TempBuffer));
        TempBuffer[8] = 0;
        strcat(TempBuffer, "  ");

        memcpy(LineBuffer + LineOffset, TempBuffer, strlen(TempBuffer));
        LineOffset += (int)strlen(TempBuffer);
        TempPtr = DataPtr;

        for (i = 0; i < ToPrint; i++) {
            _ByteToAsciiHex((unsigned char *)TempBuffer, 2, *TempPtr++);

            TempBuffer[2] = 0;
            strcat(TempBuffer, " ");

            memcpy(LineBuffer + LineOffset, TempBuffer, strlen(TempBuffer));
            LineOffset += (int) strlen(TempBuffer);

            if (i == 7) {
                memcpy(LineBuffer + LineOffset, "- ", 2);
                LineOffset += 2;
            }
        }

        LineOffset = indent + 69;

        TempPtr = DataPtr;
        for (i = 0; i < ToPrint; i++) {
            if (isprint((char)*TempPtr)) {
                TempBuffer[0] =  *TempPtr;
                TempBuffer[1] = 0;
            } else {
                TempBuffer[0] =  '.';
                TempBuffer[1] = 0;
            }

            ++TempPtr;

            memcpy(LineBuffer + LineOffset, TempBuffer, strlen(TempBuffer));
            LineOffset += (int) strlen(TempBuffer);
        }

        LineBuffer[LineOffset] = 0;

        Serial.println(LineBuffer) ;

        DataPtr += ToPrint;
        BytesRemaining -= ToPrint;
    }

    Serial.println("") ;
}

String getIPAddress (AsyncClient* client) {
    uint32_t   ip2  = client->getRemoteAddress() ;
    uint8_t  * pip2 = (uint8_t *) &ip2 ;
    char       address[(3 + 1) * 4] ;
    sprintf(address, "%d.%d.%d.%d", pip2[0], pip2[1], pip2[2], pip2[3]) ;
    // Serial.printf("\r\nRequest from [%s]\r\n", address);

    return (String(address)) ;
}

static void handleTraceData(void* arg, AsyncClient* client, const void *data, size_t len) {
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nData from %s\r\n", from);
    String from = getIPAddress(client) ;
    const unsigned char * p = (unsigned char *) data ;
    int currentLen = 0 ;

    while (len > 0) {
        switch (p[currentLen]) {
        case 0x0D :
            ++currentLen ;
            --len ;

            dump_data(from.c_str(),
                      p,
                      currentLen,
                      3) ;

            p += currentLen ;
            currentLen = 0 ;
            break ;

        default :
            ++currentLen ;
            --len ;

            if (len == 0) {
                if (currentLen > 0) {
                    // need to dump what's left in the buffer
                    dump_data(from.c_str(),
                              p,
                              currentLen,
                              3) ;
                }
            }
            break ;
        }
    }
}

 /* clients events */
static void handleTraceError(void* arg, AsyncClient* client, int8_t error) {
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nConnection Error from %s\r\n", from);
}

static void handleTraceDisconnect(void* arg, AsyncClient* client) {
    Serial.printf("Trace client %s disconnected\r\n", getIPAddress(client).c_str());
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nConnection Error from %s\r\n", from);
}

static void handleTraceTimeOut(void* arg, AsyncClient* client, uint32_t time) {
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nAck Timeout from %s\r\n", from);
}

static void handleNewTrace(void* arg, AsyncClient* client) {
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nNew connection from %s\r\n", from);

    // register events
    client->onData       (&handleTraceData,       NULL);
    client->onError      (&handleTraceError,      NULL);
    client->onDisconnect (&handleTraceDisconnect, NULL);
    client->onTimeout    (&handleTraceTimeOut,    NULL);
}

/***********************************************************************/
/***********************************************************************/
//                      Trace Services End
/***********************************************************************/
/***********************************************************************/


/***********************************************************************/
/***********************************************************************/
//                      Registration Services Start
/***********************************************************************/
/***********************************************************************/
// Client map
static std::map<String, String> clients ; // a list to hold all clients

static void handleError(void* arg, AsyncClient* client, int8_t error) {
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // Serial.printf("\r\nConnection Error from %s\r\n", from);
}

static void handleRegRequest(void* arg, AsyncClient* client, void *data, size_t len) {
    // Don't use these they don't work
    // const char * from = (const char *) client->remoteIP().toString().c_str() ;
    // AsyncClient* tmpclient = (AsyncClient*) arg ;
    // const char * from = (const char *) tmpclient->remoteIP().toString().c_str() ;
    // IPAddress  ip   = client->remoteIP() ;
    String remoteIP = getIPAddress(client) ;

    // Serial.printf("\r\nRequest from [%s]\r\n", remoteIP.c_str());
    //
    // dump_data(remoteIP.c_str(),
    //           data,
    //           len,
    //           9) ;

    char * req = (char *) data ;

    char reply[128] = {} ;
    int  replyLen = 0 ;

    switch (*req) {
    case '+' :
        {
            // wants to add register
            char * cname = (char *) &req[1] ;
            String name{cname} ;
            auto fnd = clients.find(name) ;
            if (fnd == clients.end()) {
                // doesn't exist - add it
                clients.insert(std::pair<String, String>(name, remoteIP)) ;
                Serial.printf("\r\nRegistration from %s %s\r\n", cname, remoteIP.c_str());
            }
            else {
                // Need to delete it first
                clients.erase(fnd) ;
                clients.insert(std::pair<String, String>(name, remoteIP)) ;
                Serial.printf("\r\nRe-Registration from %s %s\r\n", cname, remoteIP.c_str());
            }

            reply[0] = 'A' ;
            replyLen = 1 ;

            if (client->space() > replyLen && client->canSend()) {
                client->add((const char *)reply, replyLen);
                client->send();
            }
        }
        break ;

    case '?' :
        {
            char * cname = (char *) &req[1] ;
            String name{cname} ;

            auto fnd = clients.find(name) ;
            if (fnd == clients.end()) {
                // Serial.printf("\r\nDidn't find %s\r\n", cname);
                reply[0] = '!' ;
                replyLen = 1 ;
            }
            else {
                // Serial.printf("\r\nFind %s\r\n", cname);
                reply[0] = '=' ;
                replyLen = 1 ;

                String ip = fnd->second ;
                // Serial.printf("\r\nFound %s = ip %s\r\n", cname, ip.c_str());

                memcpy (&reply[1], ip.c_str(), strlen(ip.c_str())) ;
                replyLen += strlen(ip.c_str()) ;
                reply[replyLen] = 0 ;
            }

            if (client->space() > replyLen && client->canSend()) {
                client->add((const char *)reply, replyLen);
                client->send();
            }
        }
        break ;

        case '-' :
        {
            // wants to deregister
            char * cname = (char *) &req[1] ;
            String name{cname} ;

            auto fnd = clients.find(name) ;
            if (fnd == clients.end()) {
                clients.erase(fnd) ;
                Serial.printf("\r\nDeregistration from %s %s\r\n", cname, remoteIP);
            }

            reply[0] = 'A' ;
            replyLen = 1 ;

            if (client->space() > replyLen && client->canSend()) {
                client->add((const char *)reply, replyLen);
                client->send();
            }
        }
        break ;
    }
}

static void handleDisconnect(void* arg, AsyncClient* client) {
    Serial.printf("Register client %s disconnected\r\n", getIPAddress(client).c_str());
    // String remoteIP = getIPAddress(client) ;

    // If the disconnect is a registered entity - renmove it from our list
    // for (auto it = clients.begin(); it != clients.end(); it++) {
    //     if (it->second == remoteIP) {
    //         clients.erase(it) ;
    //         break  ;
    //     }
    // }
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
    // Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}

static void handleNewClient(void* arg, AsyncClient* client) {
    // Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());

    // register events
    client->onData(&handleRegRequest,       client) ;
    client->onError(&handleError,           client) ;
    client->onDisconnect(&handleDisconnect, client) ;
    client->onTimeout(&handleTimeOut,       client) ;
}

/***********************************************************************/
/***********************************************************************/
//                      Registration Services End
/***********************************************************************/
/***********************************************************************/
void setup() {
    Serial.begin(115200);
    delay(20);
    Serial.println("Main Tracepoint") ;

    // create access point
    while (!WiFi.softAP(LocalNetworkSys::SSID, LocalNetworkSys::PASSWORD, 6, false, 15)) {
        delay(500);
    }
    IPAddress myIP = WiFi.softAPIP();
    wifi_set_sleep_type(NONE_SLEEP_T); // Stop the power management putting the wifi to sleep  LIGHT_SLEEP_T and MODE_SLEEP_T

    // Insert the base server address
    clients.insert(std::pair<String, String>(LocalNetworkSys::TRACE_SERVER_NAME, myIP.toString())) ;

    // start dns server
    if (!DNS.start(LocalNetworkSys::DNS_PORT, LocalNetworkSys::SERVER_HOST_NAME, WiFi.softAPIP())) {
        Serial.printf("failed to start dns service\r\n");
    }

    AsyncServer* server = new AsyncServer(LocalNetworkSys::IP_LOOKUP_TCP_PORT); // start listening on tcp port 7050
    server->onClient(&handleNewClient, server);
    server->begin();

    AsyncServer* traceServer = new AsyncServer(LocalNetworkSys::TRACE_PORT); // start listening on tcp port 7050
    traceServer->onClient(&handleNewTrace, traceServer);
    traceServer->begin();
}

void loop() {
    DNS.processNextRequest();
}



