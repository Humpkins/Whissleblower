#include <PubSubClient.h>
PubSubClient mqtt(GSMclient);

#include "../6.OTA/OTA.h"

#ifndef TINY_GSM_MODEM_SIM7000
    #include "../4.SIM7000G/SIM7000G.h"
#endif

#ifndef RECONNECT_ATTEMPT_LIMIT
    #define RECONNECT_ATTEMPT_LIMIT 3
#endif

#define TIME_BETWEEN_RECONNECTS_ATTEMPTS 1000
#define RECONNECT_THREAD_TIME 10000
#define BUFFER_SIZE_PUBLISH 2048

void someoneIsListenToUs(  char * topic, byte * payload, unsigned int length );

class MQTT {

    public:
        int32_t subscribedLastHeartBeat = 0;
        int32_t idSecret = 0;
        char connectedMQTTID[25];

        // Function to setup the first connection with the MQTT Broker
        void setup() {

            mqtt.setServer( g_states.MQTTHost, g_states.MQTTPort );
            mqtt.setCallback( someoneIsListenToUs );

            Serial.print("Connecting to MQTT Host ");
            Serial.printf("%s:%i", g_states.MQTTHost, g_states.MQTTPort);

            Serial.print(" with the following client ID ");
            srand(1573);
            this->idSecret = random();
            Serial.printf("%s%llX%i", g_states.MQTTclientID, ESP.getEfuseMac(), this->idSecret);
            sprintf(this->connectedMQTTID, "%s%llX", g_states.MQTTclientID, ESP.getEfuseMac(), this->idSecret);

            // Set the max buffer size for transfering packets
            mqtt.setBufferSize(BUFFER_SIZE_PUBLISH);

            if ( !mqtt.connect( this->connectedMQTTID, g_states.MQTTUsername, g_states.MQTTPassword ) ){
                Serial.println("       [FAIL]");
                Serial.println("[ERROR]    Handdle Broker connection fail");
                utilities.ESPReset();
                while(1);
            } else {
                Serial.println("       [OK]");

                //  Subscribe to commands topic
                char topic_Wake[sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTListenTopic) + 3];
                sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTListenTopic );

                //  Subscribe to heartbeat signal topic
                char topic_HeartBeat[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTHeartBeatTopic) + 3];
                sprintf( topic_HeartBeat, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTHeartBeatTopic );

                Serial.print("Subscribing to ");
                Serial.print(topic_Wake);
                if ( !mqtt.subscribe(topic_Wake, 0) ) Serial.println("      [FAIL]");
                else Serial.println("      [OK]");

                Serial.print("Subscribing to ");
                Serial.print(topic_HeartBeat);
                if ( !mqtt.subscribe(topic_HeartBeat, 0) ) Serial.println("      [FAIL]");
                else Serial.println("      [OK]");
            }

        }

        // Function to reconect to MQTT broker if it is disconnected
        void maintainMQTTConnection() {
            // If there is no mqtt conection, then try to connect

            for ( int attempt = 0; attempt < RECONNECT_ATTEMPT_LIMIT; attempt++ ){
                Serial.print(attempt);
                Serial.print("  Reconecting to MQTT broker");

                if ( mqtt.connect( this->connectedMQTTID, g_states.MQTTUsername, g_states.MQTTPassword ) ){
                    Serial.println("       [OK]");
                    
                    mqtt.setCallback( someoneIsListenToUs );
                    mqtt.setBufferSize(BUFFER_SIZE_PUBLISH);

                    //  Subscribe to commands topic
                    char topic_Wake[sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTListenTopic) + 3];
                    sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTListenTopic );
                    mqtt.subscribe(topic_Wake, 0);

                    //  Subscribe to heartbeat signal topic
                    char topic_HeartBeat[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTHeartBeatTopic) + 3];
                    sprintf( topic_HeartBeat, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTHeartBeatTopic );
                    mqtt.subscribe(topic_HeartBeat, 0);

                    return;
                } else Serial.println("       [FAIL]");
                
                this->printMQTTstatus();
            }

            Serial.println("[ERROR]    Handdle Broker re-connection fail");
            utilities.ESPReset(  );
            while(1);
        }

        void printMQTTstatus(){
            const int state = mqtt.state();

            switch( state ){
                case -4:
                    Serial.println("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time");
                    break;
                case -3:
                    Serial.println("MQTT_CONNECTION_LOST - the network connection was broken");
                    break;
                case -2:
                    Serial.println("MQTT_CONNECT_FAILED - the network connection failed");
                    break;
                case -1:
                    Serial.println("MQTT_DISCONNECTED - the client is disconnected cleanly");
                    break;
                case 0:
                    Serial.println("MQTT_CONNECTED - the client is connected");
                    break;
                case 1:
                    Serial.println("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT");
                    break;
                case 2:
                    Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier");
                    break;
                case 3:
                    Serial.println("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection");
                    break;
                case 4:
                    Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected");
                    break;
                case 5:
                    Serial.println("MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect");
                    break;
                    
                default:
                    Serial.println("Non detected status");
                    break;
            }
        }

};

MQTT mqtt_com;

//  Callback function
void someoneIsListenToUs(  char * topic, byte * payload, unsigned int length ) {

    //  Convert the incomming byte array
    char * incommingPayload = reinterpret_cast<char*>(payload);
    incommingPayload[length] = '\0';

    char topic_Wake[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTListenTopic) + 3 ];
    sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTListenTopic );

    char topic_HeartBeat[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTHeartBeatTopic) + 3];
    sprintf( topic_HeartBeat, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTHeartBeatTopic );

    //  If it is related to web client's command messages
    if ( strcmp( topic, topic_Wake ) == 0 ) {

        if ( strcmp( incommingPayload, "getUp!" ) == 0 ) {

            //  Check this "getUp!" message as last client's hartbeat (must do it on each incomming payloads)
            mqtt_com.subscribedLastHeartBeat = xTaskGetTickCount();
            
            int status = wb.startWhistleblowing();
            switch ( status ) {
                case 1:
                    Serial.println("        [OK]");
                    if ( mqtt.publish( topic_Wake, "Yes master! Resuming the message system" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
                case 2:
                    Serial.println("        [FAIL]");
                    Serial.println("Error on opening the whistleblowing service log file");
                    if ( mqtt.publish( topic_Wake, "Error on opening the whistleblowing service log file" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
                case 0:
                    Serial.println("        [FAIL]");
                    Serial.println("Whistleblowing service is already active");
                    if ( mqtt.publish( topic_Wake, "Whistleblowing servie is already active" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
                default:
                    Serial.println("        [FAIL]");
                    if ( mqtt.publish( topic_Wake, "Error on starting the whistleblowing service" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
            }

        } else if ( strcmp( incommingPayload, "goSleep" ) == 0 ) {

            //  Check this "getUp!" message as last client's hartbeat (must do it on each incomming payloads)
            mqtt_com.subscribedLastHeartBeat = xTaskGetTickCount();

            
            Serial.print("Suspending the message system");
            int status = wb.stopWhistleblowing();
            switch( status ) {
                case 1:
                    Serial.println("        [OK]");
                    if ( mqtt.publish( topic_Wake, "Yes master! Suspending the message system" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                case 2:
                    Serial.println("        [FAIL]");
                    Serial.println("Error on opening the whistleblowing service log file");
                    if ( mqtt.publish( topic_Wake, "Error on opening the whistleblowing service log file" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                case 0:
                    Serial.println("        [FAIL]");
                    Serial.println("Whistleblowing service is already inactive");
                    if ( mqtt.publish( topic_Wake, "Whistleblowing servie is already inactive" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
                default:
                    Serial.println("        [FAIL]");
                    if ( mqtt.publish( topic_Wake, "Error on starting the whistleblowing service" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                    break;
            }

        } else if ( strcmp( incommingPayload, "resetUrSelf" ) == 0 ) {
            
            // mqtt.publish( topic_Wake, "Yes master! restarting the system. I'll be back in 1 minute" );
            Serial.println("Restarting the system. I'll be back in 1 minute");

            vTaskDelay( 1000 / portTICK_PERIOD_MS );

            utilities.ESPReset();

        } else if ( strcmp( incommingPayload, "updateUrSelf" ) == 0 ) {

            // mqtt.publish( topic_Wake, "Yes master! Starting FOTA sequence" );
            Serial.println("Starting FOTA sequence");

            OTA.loop();

        } else if ( strcmp( incommingPayload, "startRecording" ) == 0 ) {

            //  Check this "getUp!" message as last client's hartbeat (must do it on each incomming payloads)
            mqtt_com.subscribedLastHeartBeat = xTaskGetTickCount();

            if ( !SDlogger.recording ) {
                int recordingStatus = 0;

                //  Stating the logger service
                if ( xSemaphoreTake( xSD, portMAX_DELAY ) == pdTRUE ) {
                    recordingStatus = SDlogger.startLog();
                    xSemaphoreGive( xSD );
                }

                //  Resume the logger service
                switch ( recordingStatus ){
                    case 0:
                        if ( mqtt.publish( topic_Wake, "Error on start recording" ) ) { sim_7000g.last_message = xTaskGetTickCount(); }
                        Serial.println("SD mounting error");
                        break;

                    case 1:
                        if ( mqtt.publish( topic_Wake, "Yes master! Starting to record data" ) ) { sim_7000g.last_message = xTaskGetTickCount(); }
                        Serial.println("Starting to record data");
                        break;

                    case 2:
                        if ( mqtt.publish( topic_Wake, "Master, recording service is already running" ) ) { sim_7000g.last_message = xTaskGetTickCount(); }
                        Serial.println("Starting to record data");
                        break;

                    case 3:
                        if ( mqtt.publish( topic_Wake, "Error on C" ) ) { sim_7000g.last_message = xTaskGetTickCount(); }
                        Serial.println("File creation error");
                        break;
                    
                    default:
                        if ( mqtt.publish( topic_Wake, "Error on start recording" ) ) { sim_7000g.last_message = xTaskGetTickCount(); }
                        Serial.println("SD mounting error");
                        break;
                }
            }

        } else if ( strcmp( incommingPayload, "stopRecording" ) == 0 ) {

            //  Check this "getUp!" message as last client's hartbeat (must do it on each incomming payloads)
            mqtt_com.subscribedLastHeartBeat = xTaskGetTickCount();

            if ( SDlogger.recording ) {

                if (mqtt.publish( topic_Wake, "Yes master! Suspending recording service" )){ sim_7000g.last_message = xTaskGetTickCount(); }
                Serial.println("Suspending recording service");
                
                //  Suspend the logger service
                if ( xSemaphoreTake( xSD, portMAX_DELAY ) == pdTRUE ) {
                    SDlogger.stopLog();
                    xSemaphoreGive( xSD );
                }

            }
            
        } else if ( strcmp( incommingPayload, "bringServerUp_AP" ) == 0 ) {
            ESPServer.setup(true);
        } else if ( strcmp( incommingPayload, "bringServerUp_STA" ) == 0 ) {
            ESPServer.setup(false);
        } else if ( strcmp( incommingPayload, "bringServerDown" ) == 0 ) {
            ESPServer.turnESPServerOff();
        }
    }

    // If it is related to web client's heartbeat signal
    if ( strcmp( topic, topic_HeartBeat ) == 0 ) {
        mqtt_com.subscribedLastHeartBeat = xTaskGetTickCount();
    }

    return;
}