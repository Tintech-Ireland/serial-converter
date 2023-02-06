#include <ESPAsyncTCP.h>

namespace LocalNetworkSys {
const char * SSID              = "ESP-TEST"  ;
const char * PASSWORD          = "123456789" ;

const char * SERVER_HOST_NAME  = "esp_server";
const char * TRACE_SERVER_NAME = "MAINTRACE" ;

const int  IP_LOOKUP_TCP_PORT = 7050 ;
const int  DNS_PORT = 53   ;

const int  TRACE_PORT = 7052 ;

class GetServerAddress {
public:
    void connection (AsyncClient* client) {
        // Ask where this node is
        char reply[128]={} ;
        int  replyLen = 0 ;

        reply[0] = '?' ;
        ++replyLen ;

        strcpy(&reply[1], _nodeName) ;
        replyLen += strlen(_nodeName) ;
        reply [replyLen] = 0 ;
        ++replyLen ;

        // send data
        if (client->space() > replyLen && client->canSend()) {
            client->add(reply, replyLen);
            client->send();
        }
    }

    void response(AsyncClient* client, void *data, unsigned int len) {
        char * info = (char *) data ;

        switch (info[0]) {
        case '!' : // not found
            _complete = -1 ;
            break ;

        case '=' : // got it
            {
                info[len] = 0 ;
                if (_nodeIP.fromString(&info[1])) {
                    // Serial.printf("Server IP = %s\r\n", _nodeIP.toString().c_str());
                }
                else {
                    // Serial.printf("IP Address import failed\r\n");
                }
                _complete = 1 ;
            }
            break ;

        }
        // Serial.println("\r\n============================") ;
    }

    static void onConnect (void* arg, AsyncClient* client) {
        GetServerAddress * _gsv = (GetServerAddress *) arg ;
        _gsv->connection(client) ;
    }

    static void handleData(void* arg, AsyncClient* client, void *data, unsigned int len) {
        GetServerAddress * _gsv = (GetServerAddress *) arg ;
        _gsv->response(client, data, len) ;
    }

    GetServerAddress (const char * DNSServerName, int port, const char * nodeName) : _complete{0} {
        assert (DNSServerName != NULL) ;
        assert (nodeName != NULL) ;

        strcpy(_nodeName, nodeName) ;

        _client.onData(static_cast<AcDataHandler>(handleData),      (void *)this);
        _client.onConnect(static_cast<AcConnectHandler>(onConnect), (void *)this);

        _client.connect(DNSServerName, port);
    }

    ~GetServerAddress() {
        _client.close() ;
    }

    bool waitComplete (unsigned long waitMs) {
        // wait for the result
        unsigned long end = millis() + waitMs ;
        while ((_complete == 0) && millis() < end) {
            delay(50) ;
        }

        return _complete ;
    }

    IPAddress ipFound (void) {
        return (_nodeIP) ;
    }

private:
    AsyncClient     _client ;
    char            _nodeName[128] ;
    IPAddress       _nodeIP ;
    int             _complete ;

};

class RegisterServer {
public:
    RegisterServer (const char * host, int port, const char * serverName) : _success{false} {
        assert (host != NULL) ;
        assert (serverName != NULL) ;
        assert (strlen(serverName) < 100) ; // don't allow ridiculously long names

        if (_client.connect(host, port)) {
            // send the register request
            unsigned char buffer [128] ;
            int           bufferLen = 0 ;

            buffer[bufferLen] = '+' ;
            ++bufferLen ;
            memcpy (&buffer[bufferLen], serverName, strlen(serverName)) ;
            bufferLen += strlen(serverName) ;
            buffer[bufferLen] = 0 ;
            ++bufferLen ; // include the NULL pointer

            _client.write (buffer, bufferLen) ;

            _success = true ;
        }
    }

    ~RegisterServer() {
        _client.stop() ;
    }

    bool registered (void) {
        return _success ;
    }

private:
    bool         _success ;
    WiFiClient   _client  ;

} ;

class SystemServer {
public:
    SystemServer (const char * DNSServerName, int DNSport, int serverPort, const char * nodeName) : _serverRegister{NULL} {
        assert (DNSServerName != NULL) ;
        assert (nodeName != NULL) ;

        LocalNetworkSys::GetServerAddress gsv(DNSServerName, DNSport, TRACE_SERVER_NAME) ;

        if (gsv.waitComplete (10000) > 0) {
            _traceServerIP = gsv.ipFound() ;

            _serverRegister = new RegisterServer (DNSServerName, DNSport, nodeName) ;
            assert (_serverRegister != NULL) ;

            _traceClient = new WiFiClient () ;
            assert (_traceClient != NULL) ;

            if (_serverRegister->registered()) {
                if (_traceClient->connect(_traceServerIP, TRACE_PORT)) {
                    _server = new AsyncServer(serverPort);

                    if (_server != NULL) {
                        _server->onClient(&handleNewClient, this);
                        _server->begin();
                    }
                }
            }
        }
    }

    virtual ~SystemServer() {
        if (_server != NULL)  {
            delete _server ;
            _server = NULL ; // superfluous but makes me feel better
        }

        if (_traceClient != NULL) {
            delete _traceClient ;
            _traceClient = NULL ; // superfluous but makes me feel better
        }
    }

protected:
    IPAddress        _traceServerIP  ;
    RegisterServer * _serverRegister ;
    WiFiClient     * _traceClient    ;
    AsyncServer*     _server         ;

    void trace (void * data, int dataLen) {
        assert (data != NULL) ;

        if (dataLen > 0) {
            if (!_traceClient->connected()) {
                _traceClient->connect(_traceServerIP, TRACE_PORT) ;
            }
            _traceClient->write ((unsigned char *)data, dataLen) ;
        }
    }

    void trace (char * data) {
        assert (data != NULL) ;
        trace (data, strlen(data)) ;
    }

    static void handleError(void* arg, AsyncClient* client, int8_t error) {
        // char * from = client->remoteIP().toString().c_str()
    }

    static void handleDisconnect(void* arg, AsyncClient* client) {
        // char * from = client->remoteIP().toString().c_str()
        SystemServer * _gsv = (SystemServer *) arg ;
        _gsv->disconnection (client) ;
    }

    static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
        // char * from = client->remoteIP().toString().c_str()
    }

    void setupCallbacks (AsyncClient* client) {
        // Serial.printf("Here okay %s\r\n", client->remoteIP().toString().c_str());
        // char * from = client->remoteIP().toString().c_str()
        // register events
        client->onData       (&handleData,       this);
        client->onError      (&handleError,      this);
        client->onDisconnect (&handleDisconnect, this);
        client->onTimeout    (&handleTimeOut,    this);
    }

    static void handleNewClient (void* arg, AsyncClient* client) {
        assert (arg != NULL) ;
        assert (client != NULL) ;
        // Serial.printf("new client has been connected to server, ip: %s\r\n", client->remoteIP().toString().c_str());

        SystemServer * _gsv = (SystemServer *) arg ;
        _gsv->connection(client) ;
    }

    static void handleData(void* arg, AsyncClient* client, void *data, unsigned int len) {
        assert (arg != NULL) ;
        assert (client != NULL) ;

        // Serial.printf("Data sent to server from ip: %s\r\n", client->remoteIP().toString().c_str());
        SystemServer * _gsv =  (SystemServer *) arg ;
        _gsv->response(client, data, len) ;
    }

    //********************************************************************
    //********************************************************************
    //********************************************************************
    //              Virtual methods that can be overridden
    //********************************************************************
    //********************************************************************
    //********************************************************************

    virtual void disconnection (AsyncClient* client) {
        // Serial.printf("Disconnection %s\r\n", client->remoteIP().toString().c_str());
    }

    virtual void connection (AsyncClient* client) {
        // Serial.printf("New Connection %s\r\n", client->remoteIP().toString().c_str());
        // char * from = client->remoteIP().toString().c_str()
        // register events
        setupCallbacks (client) ;
    }

    virtual void response(AsyncClient* client, void *data, unsigned int len) {
        // char * from = client->remoteIP().toString().c_str()
        // Serial.printf("No override!!!: %s\r\n", client->remoteIP().toString().c_str());
    }
};

} // namespace LocalNetworkSys

