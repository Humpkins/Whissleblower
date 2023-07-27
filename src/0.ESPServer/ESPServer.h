#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

// #define IS_WIFI_AP
// #ifndef IS_WIFI_AP
//     WiFiClient wifiClient;
// #endif

AsyncWebServer server(80);

class ESP_Server {

    public:
        int16_t maxReturnedFiles = 5;
        int16_t ReturnedFilesCount = 0;
        TickType_t lastRequest = xTaskGetTickCount();
        int16_t isWebServerOn = 0;
        int16_t is_wifi_AP_value = 0;

        void setup( int16_t is_wifi_AP ){
            if ( !this->isWebServerOn ) {
                this->isWebServerOn = 1;

                #ifdef SPIFFS_H
                SPIFFS.begin(true);
                #endif

                Serial.println("Setting up the ESP Server");

                this->is_wifi_AP_value = is_wifi_AP;
                if ( is_wifi_AP ) {
                    WiFiClient wifiClient;

                    Serial.println("Connecting on Access Point mode");
                    Serial.printf("SSID: %s PWD: %s \n", g_states.AP_SSID, g_states.AP_PSW);
                    // WiFi.mode(WIFI_AP);
                    WiFi.softAP( g_states.AP_SSID, g_states.AP_PSW );
                    TickType_t start = xTaskGetTickCount();
                    while( WiFi.status() != WL_CONNECTED ){
                        Serial.print(".");
                        if ( (xTaskGetTickCount() - start) > (60000 / portTICK_PERIOD_MS) ) {
                            Serial.println("\n        [FAIL]");
                            this->turnESPServerOff();
                            return;
                        }
                        vTaskDelay( 1000/portTICK_PERIOD_MS );
                    }
                    Serial.println("\n       [OK]");

                    Serial.print("Connected AP ");

                } else {
                    Serial.println("Connecting on Station mode");
                    Serial.printf("SSID: %s PWD: %s \n", g_states.STA_SSID, g_states.STA_PSW);
                    WiFi.mode(WIFI_STA);
                    WiFi.begin( g_states.STA_SSID, g_states.STA_PSW );
                    TickType_t start = xTaskGetTickCount();
                    while( WiFi.status() != WL_CONNECTED ){
                        Serial.print(".");
                        if ( (xTaskGetTickCount() - start) > (60000 / portTICK_PERIOD_MS) ) {
                            Serial.println("        [FAIL]");
                            this->turnESPServerOff();
                            return;
                        }
                        vTaskDelay( 1000/portTICK_PERIOD_MS );
                    }
                    Serial.println("\n       [OK]");

                    Serial.print("Connected STA with the following IP: ");
                    Serial.println(WiFi.localIP().toString().c_str());
                }

                if (!MDNS.begin(g_states.HOST)){
                    Serial.println("[ERROR]    Handdle MDNS fail");
                } else {
                    Serial.print("mDNS: ");
                    Serial.println(g_states.HOST);
                }

                DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
                DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT");
                DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

                //  CORS Handdle
                server.onNotFound( []( AsyncWebServerRequest *request ){
                    if ( request->method() == HTTP_OPTIONS )
                        request->send(204);
                    else
                        request->send(404);
                });

                //  Requests for the current configuration data
                server.on(
                    "/info",
                    HTTP_GET,
                    [this]( AsyncWebServerRequest *request ){
                        this->lastRequest = xTaskGetTickCount();
                        
                        StaticJsonDocument<512> rootDoc;

                        Serial.println("/info requested...");
                        AsyncResponseStream *response = request -> beginResponseStream("application/json");

                        rootDoc["APN"]["host"] = g_states.APN_GPRS;
                        rootDoc["APN"]["user"] = g_states.APNUser;
                        rootDoc["APN"]["password"] = g_states.APNPassword;

                        rootDoc["MQTT"]["host"] = g_states.MQTTHost;
                        rootDoc["MQTT"]["user"] = g_states.MQTTUsername;
                        rootDoc["MQTT"]["password"] = g_states.MQTTPassword;
                        rootDoc["MQTT"]["port"] = g_states.MQTTPort;
                        rootDoc["MQTT"]["ID"] = g_states.MQTTclientID;
                        rootDoc["MQTT"]["project"] = g_states.MQTTProject;

                        rootDoc["WiFi"]["STA"]["ssid"] = g_states.STA_SSID;
                        rootDoc["WiFi"]["STA"]["password"] = g_states.STA_PSW;

                        rootDoc["WiFi"]["AP"]["ssid"] = g_states.AP_SSID;
                        rootDoc["WiFi"]["AP"]["password"] = g_states.AP_PSW;

                        serializeJson( rootDoc, *response );

                        request->send(response);
                    }
                );

                //  Requests for all log files
                server.on(
                    "/logData",
                    HTTP_GET,
                    [this]( AsyncWebServerRequest *request ) {
                        this->lastRequest = xTaskGetTickCount();
                        
                        StaticJsonDocument<1051> rootDoc;
                        JsonArray logs = rootDoc.createNestedArray("logsNames");

                        Serial.println("/logData requested...");
                        AsyncResponseStream *response = request -> beginResponseStream("application/json");

                        if ( xSemaphoreTake( xSD, ( 10000 / portTICK_RATE_MS ) ) == pdTRUE ) {

                            //  Check if SD is mounted
                            if ( !sd_card.mountSD() ) {
                                Serial.println("Error SD mounting");
                                request->send(500);
                                return;
                            }

                            File root = SD.open("/");
                            if(!root){
                                Serial.println("Failed to open directory");
                                request->send(500);
                                return;
                            }
                            if(!root.isDirectory()){
                                Serial.println("Not a directory");
                                request->send(500);
                                return;
                            }
                                
                            this->ReturnedFilesCount = 0;
                            File file = root.openNextFile();
                            while (file) {

                                if( !file.isDirectory() ){
                                    char *name = (char *)file.name();
                                    size_t size = file.size();

                                    if ( size > 0  && strstr( name, ".txt" ) != NULL ) {

                                        JsonObject log = logs.createNestedObject();

                                        log["name"] = name;
                                        log["size"] = size;

                                        logs.add( log );

                                        this->ReturnedFilesCount++;
                                    }
                                }

                                file = root.openNextFile();

                                if ( this->ReturnedFilesCount > this->maxReturnedFiles ) break;
                            };

                            root.close();

                            xSemaphoreGive( xSD );

                            serializeJson( rootDoc, *response );

                            request->send(response);
                        }

                        request->send( 500 );
                        return;

                    }
                );

                //  Upload file to web client
                server.on(
                    "/download",
                    HTTP_GET,
                    [this]( AsyncWebServerRequest *request ) {
                        this->lastRequest = xTaskGetTickCount();

                        if ( request->hasParam("name") ) {
                            AsyncWebParameter* params = request->getParam("name");
                            const char * fileName = params->value().c_str();
                            
                            char FullFilePath[ sizeof( fileName ) + 2 ];
                            sprintf( FullFilePath, "/%s", fileName );
                            
                            if ( xSemaphoreTake( xSD, (10000/portTICK_PERIOD_MS) ) == pdTRUE ) {
                                //  Check if SD is mounted
                                if ( !sd_card.mountSD() ) {
                                    Serial.println("Error SD mounting");
                                    request->send(500);
                                }

                                if ( !SD.exists(FullFilePath) ) request->send(404);
                                else {
                                    Serial.printf("Serving file: %s\n", FullFilePath);
                                    File file = SD.open(FullFilePath);
                                    if ( file ) {
                                        AsyncWebServerResponse *response = request->beginResponse( file, FullFilePath, String(), true );
                                        
                                        request->send(response);
                                    } else request->send(404);


                                };

                                xSemaphoreGive( xSD );
                            };
                        } else request->send(404);
                    }
                );

                //  Upload file to web client
                server.on(
                    "/delete",
                    HTTP_DELETE,
                    [this]( AsyncWebServerRequest *request ) {
                        this->lastRequest = xTaskGetTickCount();

                        if ( request->hasParam("name") ) {
                            AsyncWebParameter* params = request->getParam("name");
                            const char * fileName = params->value().c_str();
                            
                            char FullFilePath[ sizeof( fileName ) + 2 ];
                            sprintf( FullFilePath, "/%s", fileName );
                            if ( xSemaphoreTake( xSD, ( 10000 / portTICK_PERIOD_MS ) ) == pdTRUE ) {
                                //  Check if SD is mounted
                                if ( !sd_card.mountSD() ) {
                                    Serial.println("Error SD mounting");
                                    request->send(500);
                                }

                                if ( !SD.exists(FullFilePath) ) request->send(404);
                                else {
                                    Serial.printf("/deleting %s\n", fileName);
                                    SD.remove(FullFilePath);
                                    request->send(200);
                                }

                                xSemaphoreGive( xSD );
                            }
                        }
                    }
                );

                //  Upload file to web client
                server.on(
                    "/isServerUp",
                    HTTP_GET,
                    [this]( AsyncWebServerRequest *request ) {
                        this->lastRequest = xTaskGetTickCount();

                        Serial.println("/isServerUp requested...");
                        request->send(200);
                    }
                );

                #ifdef SPIFFS_H
                //  Saves data to SPIFFS
                server.on(
                    "/saveData",
                    HTTP_GET,
                    [this]( AsyncWebServerRequest *request ){
                        this->lastRequest = xTaskGetTickCount();

                        Serial.println("/saveData requested...");

                        AsyncWebParameter *paramAPN_GPRS;
                        char APN_GPRS[31];
                        if ( request->hasParam("APN_GPRS") ) {
                            paramAPN_GPRS = request->getParam("APN_GPRS");
                            sprintf( APN_GPRS, "%s", (char *)paramAPN_GPRS->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramAPNUser;
                        char APNUser[31];
                        if ( request->hasParam("APNUser") ) {
                            paramAPNUser = request->getParam("APNUser");
                            sprintf( APNUser, "%s", (char *)paramAPNUser->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramAPNPassword;
                        char APNPassword[31];
                        if ( request->hasParam("APNPassword") ) {
                            paramAPNPassword = request->getParam("APNPassword");
                            sprintf( APNPassword, "%s", (char *)paramAPNPassword->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTHost;
                        char MQTTHost[31];
                        if ( request->hasParam("MQTTHost") ) {
                            paramMQTTHost = request->getParam("MQTTHost");
                            sprintf( MQTTHost, "%s", (char *)paramMQTTHost->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTUsername;
                        char MQTTUsername[31];
                        if ( request->hasParam("MQTTUsername") ) {
                            paramMQTTUsername = request->getParam("MQTTUsername");
                            sprintf( MQTTUsername, "%s", (char *)paramMQTTUsername->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTPassword;
                        char MQTTPassword[31];
                        if ( request->hasParam("MQTTPassword") ) {
                            paramMQTTPassword = request->getParam("MQTTPassword");
                            sprintf( MQTTPassword, "%s", (char *)paramMQTTPassword->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTclientID;
                        char MQTTclientID[31];
                        if ( request->hasParam("MQTTclientID") ) {
                            paramMQTTclientID = request->getParam("MQTTclientID");
                            sprintf( MQTTclientID, "%s",(char *)paramMQTTclientID->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTProject;
                        char MQTTProject[31];
                        if ( request->hasParam("MQTTProject") ) {
                            paramMQTTProject = request->getParam("MQTTProject");
                            sprintf( MQTTProject, "%s",(char *)paramMQTTProject->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramSTA_SSID;
                        char STA_SSID[31];
                        if ( request->hasParam("STA_SSID") ) {
                            paramSTA_SSID = request->getParam("STA_SSID");
                            sprintf( STA_SSID, "%s",(char *)paramSTA_SSID->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramSTA_PSW;
                        char STA_PSW[31];
                        if ( request->hasParam("STA_PSW") ) {
                            paramSTA_PSW = request->getParam("STA_PSW");
                            sprintf( STA_PSW, "%s",(char *)paramSTA_PSW->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramAP_SSID;
                        char AP_SSID[31];
                        if ( request->hasParam("AP_SSID") ) {
                            paramAP_SSID = request->getParam("AP_SSID");
                            sprintf( AP_SSID, "%s",(char *)paramAP_SSID->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramAP_PSW;
                        char AP_PSW[31];
                        if ( request->hasParam("AP_PSW") ) {
                            paramAP_PSW = request->getParam("AP_PSW");
                            sprintf( AP_PSW, "%s",(char *)paramAP_PSW->value().c_str());
                        } else {
                            request->send(422);
                            return;
                        }

                        AsyncWebParameter *paramMQTTPort;
                        int16_t MQTTPort;
                        if ( request->hasParam("MQTTPort") ) {
                            paramMQTTPort = request->getParam("MQTTPort");
                            MQTTPort = (int16_t)paramMQTTPort->value().toInt();
                        } else {
                            request->send(422);
                            return;
                        }

                        Serial.printf(
                            "APN_GPRS: %s\nAPNUser: %s\nAPNPassword: %s\nMQTTHost: %s\nMQTTUsername: %s\nMQTTPassword: %s\nMQTTclientID: %s\nMQTTProject: %s\nMQTTPort: %i\nSTA_SSID: %s\nSTA_PSW: %s\nAP_SSID: %s\nAP_PSW: %s\n",
                            APN_GPRS,
                            APNUser,
                            APNPassword,
                            MQTTHost,
                            MQTTUsername,
                            MQTTPassword,
                            MQTTclientID,
                            MQTTProject,
                            MQTTPort,
                            STA_SSID,
                            STA_PSW,
                            AP_SSID,
                            AP_PSW
                        );

                        //  Save all collected data to SPIFFS
                        if ( g_states.saveConfigToSPIFFS(
                            APN_GPRS, APNUser, APNPassword,
                            MQTTHost, MQTTUsername, MQTTPassword,
                            MQTTPort, MQTTclientID, MQTTProject,
                            STA_SSID, STA_PSW, AP_SSID, AP_PSW
                        ) ){
                            SPIFFS.begin();
                            request->send(200);
                        } else {
                            SPIFFS.begin();
                            request->send(500);
                        }
                        
                    }
                );
                
                server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
                server.serveStatic("/favicon.ico", SPIFFS, "/").setDefaultFile("favicon.ico");

                #endif

                server.begin();

                this->lastRequest = xTaskGetTickCount();
                vTaskResume(xESPServer);
            }

        }

        //  Turn the server off
        void turnESPServerOff(){
            if ( this->isWebServerOn ) {
                this->isWebServerOn = 0;
                Serial.print("Shuting server off");

                WiFi.disconnect();
                WiFi.softAPdisconnect(true);
                MDNS.end();
                SPIFFS.end();
                server.end();

                vTaskSuspend(xESPServer);

                Serial.println("        [OK]");
            }
        }

        //  Check if server does not get any request on the last minute
        void ServerTimeout(){
            TickType_t tickDiff = xTaskGetTickCount() - this->lastRequest;

            if ( tickDiff > ( 60000 ) / portTICK_PERIOD_MS )
                this->turnESPServerOff();
        }
};


ESP_Server ESPServer;