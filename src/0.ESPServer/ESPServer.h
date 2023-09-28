#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
// #include <WiFi.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
#include "esp_system.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include <SPIFFS.h>
#include <SD.h>

AsyncWebServer server(80);

class ESP_Server {

    public:
        int16_t maxReturnedFiles = 5;
        int16_t ReturnedFilesCount = 0;
        TickType_t lastRequest = xTaskGetTickCount();
        int16_t isWebServerOn = 0;
        int16_t is_wifi_AP_value = 0;
        esp_err_t ret;

        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                            int32_t event_id, void* event_data)
        {
            if (event_id == WIFI_EVENT_AP_STACONNECTED) {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                        MAC2STR(event->mac), event->aid);
            } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                        MAC2STR(event->mac), event->aid);
            }
        }

        void setup( int16_t is_wifi_AP ){
            if ( !this->isWebServerOn ) {
                this->isWebServerOn = 1;

                esp_netif_init();
                ESP_ERROR_CHECK(esp_event_loop_create_default());
                esp_netif_create_default_wifi_ap();

                // Initialize WiFi
                wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                esp_wifi_init(&cfg);

                ESP_ERROR_CHECK(
                    esp_event_handler_instance_register(
                        WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &wifi_event_handler,
                        NULL,
                        NULL
                    )
                );

                #ifdef SPIFFS_H
                this->ret = esp_vfs_spiffs_register(&g_states.conf);
                #endif

                Serial.println("Setting up the ESP Server");

                this->is_wifi_AP_value = is_wifi_AP;
                if ( is_wifi_AP ) {

                    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                    esp_err_t err2 = esp_wifi_init(&cfg);
                    Serial.println(err2);

                    Serial.println("Connecting on Access Point mode");
                    
                    // Configure WiFi as an Access Point
                    wifi_config_t ap_config;
                    ap_config.ap.max_connection = 4;
                    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

                    // Copy the SSID and password strings to the configuration
                    strncpy((char*)ap_config.ap.ssid, "teste", sizeof(ap_config.ap.ssid) - 1);
                    strncpy((char*)ap_config.ap.password, "teste", sizeof(ap_config.ap.password) - 1);


                    Serial.printf("SSID: %s PWD: %s     ", ap_config.ap.ssid, ap_config.ap.password);

                    esp_wifi_set_mode(WIFI_MODE_AP);
                    esp_wifi_set_config(WIFI_IF_AP, &ap_config);

                    // Wait for WiFi to connect
                    uint32_t connect_timeout_ms = 60000; // Adjust as needed
                    TickType_t start_ticks = xTaskGetTickCount();
                    wifi_ap_record_t wifi_info;

                    
                    esp_wifi_start();

                    int err;
                    while (1) {
                        err = esp_wifi_sta_get_ap_info(&wifi_info);
                        if ( err == ESP_OK) {
                            break; // Exit the loop once we've printed the IP address
                        }

                        // Check for a timeout
                        if ((xTaskGetTickCount() - start_ticks) * portTICK_PERIOD_MS >= connect_timeout_ms) {
                            Serial.println("        [FAIL]");
                            Serial.println(esp_err_to_name(err));
                            this->turnESPServerOff();
                            return;
                        }

                        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay to avoid busy-waiting
                    }

                    Serial.println("       [OK]");

                } else {
                    Serial.println("Connecting on Station mode");
                    Serial.printf("SSID: %s PWD: %s     ", g_states.STA_SSID, g_states.STA_PSW);

                    // Configure WiFi as a Station
                    wifi_config_t sta_config;

                    // Copy the SSID and password strings to the configuration
                    strncpy((char*)sta_config.sta.ssid, g_states.STA_SSID, sizeof(sta_config.sta.ssid) - 1);
                    strncpy((char*)sta_config.sta.password, g_states.STA_PSW, sizeof(sta_config.sta.password) - 1);

                    esp_wifi_set_mode(WIFI_MODE_STA);
                    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
                    esp_wifi_start();

                    // Wait for WiFi to connect
                    uint32_t connect_timeout_ms = 10000; // Adjust as needed
                    TickType_t start_ticks = xTaskGetTickCount();
                    wifi_ap_record_t wifi_info;

                    while (1) {
                        // Exit the loop once we've printed the IP address
                        if (esp_wifi_sta_get_ap_info(&wifi_info) == ESP_OK) {
                            break; 
                        }

                        // Check for a timeout
                        if ((xTaskGetTickCount() - start_ticks) * portTICK_PERIOD_MS >= connect_timeout_ms) {
                            Serial.println("        [FAIL]");
                            this->turnESPServerOff();
                            return;
                        }

                        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay to avoid busy-waiting
                    }

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

                            DIR *root = opendir("/");
                            struct dirent *entry;
                            struct stat *sb;
                            int statok;
                                
                            this->ReturnedFilesCount = 0;
                            while ((entry = readdir(root)) != NULL) {

                                if( entry->d_type == DT_REG ){
                                    statok = stat( entry->d_name, sb );

                                    char *name = entry->d_name;
                                    int size = (int)sb->st_size;

                                    if ( size > 0  && strstr( name, ".txt" ) != NULL ) {

                                        JsonObject log = logs.createNestedObject();

                                        log["name"] = name;
                                        log["size"] = size;

                                        logs.add( log );

                                        this->ReturnedFilesCount++;
                                    }
                                }

                                if ( this->ReturnedFilesCount > this->maxReturnedFiles ) break;
                            };

                            closedir( root );

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

                                File downloadFile = SD.open( FullFilePath );
                                if( !downloadFile ) request->send(404);
                                else {
                                    Serial.printf("Serving file: %s\n", FullFilePath);
                                    AsyncWebServerResponse *response = request->beginResponse(
                                        downloadFile,
                                        FullFilePath,
                                        String(),
                                        true
                                    );
                                    
                                    request->send(response);
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