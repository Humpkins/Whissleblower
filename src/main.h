#include <Arduino.h>

void customAbort(){
    Serial.println("Custom abort called");
    digitalWrite( ESP_RST_PIN, LOW );
}

#define abort() customAbort();

TaskHandle_t xClientsHeartbeat;
TaskHandle_t xMediumFreq;
TaskHandle_t xHighFreq;
TaskHandle_t xMQTTDeliver;
TaskHandle_t xMQTTLoop;
TaskHandle_t xHeartBeat;
TaskHandle_t xLog;
TaskHandle_t xESPServer;
TaskHandle_t xRTC;
TaskHandle_t xVoltageIn;

SemaphoreHandle_t xModem = NULL;
SemaphoreHandle_t xSD = NULL;

void toggleHighFreq( bool state );
void toggleMediumFreq( bool state );
void toggleMQTTFreq( bool state );