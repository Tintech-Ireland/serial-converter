//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include "painlessMesh.h"
#include <base64Ing.h>

#define   MESH_PREFIX     "MeshDebug"
#define   MESH_PASSWORD   "ElephantBattery"
#define   MESH_PORT       5555

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
Base64Ing     b64;
int           bossID     = 0 ;
bool          traceReady = false;

// User stub
// void pollSerial() ; // Prototype so PlatformIO doesn't complain
// Task taskSerial(TASK_SECOND * 1, TASK_FOREVER, &pollSerial);

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
    if (memcmp(msg.c_str(), "~~~~Who boss", 12) == 0) {
        // Not for us, ignore
    } else if (memcmp(msg.c_str(), "++++I am", 8) == 0) {
        // Serial.printf("--> Received server re-registration, nodeId = %u\n", from);
        digitalWrite(LED_BUILTIN, LOW);
        // Registered
        bossID       = from ;
        traceReady   = true ;
        // taskSerial.setInterval(TASK_MILLISECOND *  20);
    }
}

void newConnectionCallback(uint32_t nodeId) {
    // Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  // Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void sendTrace (const unsigned char * data, int dataLen) {
    if (traceReady) {
        if (data != NULL) {
            char *enc = b64.b64_encode((const unsigned char *)data, dataLen);

            if (*enc != NULL) {
                // mesh.sendSingle(bossID, enc);
                mesh.sendBroadcast(enc);
            }
        }
    }
}

void sendTrace (const char * data) {
    // Serial.println("Sending TraceData") ;
    sendTrace ((const unsigned char *) data, strlen(data)) ;
}

#define RECEIVE_LEN 1000
int LINE_SPEED = 38400 ;

unsigned char receive_buffer[RECEIVE_LEN] ;
int           receive_pos   = 0 ;

int   receiveByte  (void) {
    int receivedByte = -1 ;

    if (Serial.available()) {
        receivedByte = Serial.read() ;
    }

    return (receivedByte) ;
}

void sendByte (unsigned char toSend) {
    Serial.write (&toSend, 1) ;
    sendTrace ((unsigned char *) &toSend, 1) ;
}

void sendData (unsigned char * toSend, int length) {
    if (length > 0) {
        if (toSend != NULL) {
            Serial.write (toSend, length) ;
            sendTrace ((unsigned char *) toSend, length) ;
        }
    }
}

void sendData (const char * toSend) {
    sendData ((unsigned char *) toSend, strlen(toSend)) ;
}

void setup() {
  Serial.begin(LINE_SPEED);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

 //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // request the server ID
  mesh.sendBroadcast("~~~~Who boss");

  // userScheduler.addTask( taskSerial );
  // taskSerial.enable();
}

// Can't use delay with task scheduler - it stuffs things up
void myDelay (unsigned int toWait) {
    if (toWait <= 0) toWait = 1 ;
    unsigned int end = millis() + toWait ;
    while (millis() < end) ;
}

void processCommand (char * fullCommand) {
    char * command = strtok (fullCommand, " ") ;

    if (command != NULL) {
        if (strcmp(command, "232BAUD?") == 0) {
            sendTrace( fullCommand );
            char startupstring[10] ;
            strcpy (startupstring, "#x\r") ;

            switch (LINE_SPEED) {
            case 300:
                startupstring[1] = '0';
                break;

            case 600:
                startupstring[1] = '1';
                break;

            case 1200:
                startupstring[1] = '2';
                break;

            case 2400:
                startupstring[1] = '3';
                break;

            case 4800:
                startupstring[1] = '4';
                break;

            default :
            case 9600:
                startupstring[1] = '5';
                break;

            case 19200 :
                startupstring[1] = '6';
                break;

            case 38400 :
                startupstring[1] = '7';
                break;

            case 57600 :
                startupstring[1] = '8';
                break;

            case 115200 :
                startupstring[1] = '9';
                break;
            }
            sendData (startupstring) ;
        }
        else if (strcmp(command, "232BAUD") == 0) {
            sendTrace( fullCommand );
            int speed = (int) fullCommand[8] ;

            switch (speed) {
            case '0' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(300) ;
                LINE_SPEED = 300 ;
                break ;

            case '1' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(600) ;
                LINE_SPEED = 600 ;
                break ;

            case '2' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(1200) ;
                LINE_SPEED = 1200 ;
                break ;

            case '3' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(2400) ;
                LINE_SPEED = 2400 ;
                break ;

            case '4' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(4800) ;
                LINE_SPEED = 4800 ;
                break ;

            case '5' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(9600) ;
                LINE_SPEED = 9600 ;
                break ;

            case '6' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(19200) ;
                LINE_SPEED = 19200 ;
                break ;

            case '7' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(38400) ;
                LINE_SPEED = 38400 ;
                break ;

            case '8' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(57600) ;
                LINE_SPEED = 57600 ;
                break ;

            case '9' :
                sendData ("#+\r") ;
                myDelay(50) ; // let the data transmit first

                Serial.end() ;
                Serial.begin(115200) ;
                LINE_SPEED = 115200 ;
                break ;

            default :
                sendData ("#-\r") ;
                break ;
            }
        }
        else if (strcmp(command, "BEPEXEC") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "SUFENAB") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "SUFBLOK") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "BEPGRNO") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "NORDMSG") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "TRGTIME") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "SHUTMIN") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "DECVOTE") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "BARFILT") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "NO_READ") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "SENSOPT") == 0) {
            sendTrace( fullCommand );
            sendData ("#+\r") ;
        }
        else if (strcmp(command, "REVSOFT") == 0) {
            sendTrace( fullCommand );
            sendData ("#FW-1400-01-RevK\rEV15 A41P135_ CPU 1.0\r") ;
        }
        else if (strcmp(command, "TRGON") == 0) {
            sendTrace( fullCommand );
            sendData ("801201\xFF") ;
        }
        else {
            String msg = "Unrecognised (" ;
            msg  += fullCommand  ;
            msg  += ")" ;

            sendTrace( msg.c_str() );
            // unrecognised command
            sendData ("#|\r") ;
        }
    }
}

bool collecting = false ;
void pollSerial () {
    if (!traceReady) {
        digitalWrite(LED_BUILTIN, HIGH);  // Turn off
        // Serial.println("Registering with server") ;

        // request the server ID again
        mesh.sendBroadcast("~~~~Who boss");
        myDelay(500) ; // let the data transmit first
    }

    int byteIn  ;

    do {
        byteIn = receiveByte () ;

        if (byteIn >= 0) {
            switch (byteIn) {
            case '#' :
                receive_pos = 0 ;
                collecting = true ;
                break ;

            case '\r' :
                receive_buffer[receive_pos] = 0 ; // end the string
                receive_pos = 0 ;    // reset the buffer
                collecting  = false ; // stop appending
                processCommand ((char *)receive_buffer) ;
                break ;

            default :
                if (collecting) {
                    receive_buffer [receive_pos++] = (unsigned char) byteIn ;
                }
                break ;
            }
        }

    } while (byteIn >= 0) ;
}

void loop() {
    // it will run the user scheduler as well
    mesh.update();
    pollSerial () ;
}



