#include <Arduino.h>

#define ESP_RST_PIN GPIO_NUM_5
#define ESP_BAT_PIN GPIO_NUM_35
#define TESTING_CAN

#include <main.h>

#include "secrets.h"

#include "./10.SD/SDcard.h"
#include "./5.Utils/utils.h"

#include "./config.h"

#include "./0.ESPServer/ESPServer.h"
#include "./11.AntiCrashRoutine/AntiCrash.h"
#include "./2.MPU6050/MPU6050.h"
#ifdef TESTING_CAN
        #include "./8.TJA1050/TJA1050_testing.h"
    #else
        #include "./8.TJA1050/TJA1050.h"
#endif
#include "./4.SIM7000G/SIM7000G.h"
#include "./9.LOGGER/logger.h"
#include "./12.Whistleblowing/whistleblowing.h"
#include "./3.MQTT/MQTT.h"
#include "./7.Watchers/overcurrent.h"

#include "esp_system.h"
#include "driver/gpio.h"

#include "./highFrequencyMQTT/highFrequencyMQTT.h"
#include "./mediumFrequencyMQTT/mediumFrequencyMQTT.h"
#include "./CANerrorMQTT/CANErrorMQTT.h"

#define LED_PIN     GPIO_NUM_12
#define TASK_QTY 2

void stack_overflow_handler( TaskHandle_t xTask, signed char *pcTaskName ) {
    Serial.printf("The task %s overflowed\n", pcTaskName);
    utilities.ESPReset();
};

//  Toggle the high frequency task state
void toggleHighFreq( bool state ) {
    if ( state ) vTaskResume( xHighFreq );
    else vTaskSuspend( xHighFreq );
};

//  Sends the high frequency data over mqtt
void TaskHighFreq( void * parameters ){
    //  Suspend itself and wait to be awaken
    vTaskSuspend(NULL);

    static TickType_t xDelay = g_states.MQTTHighPeriod / portTICK_PERIOD_MS;

    for (;;) {
        if ( g_states.VoltageInput || true ) {
            // Serial.print("Getting TJA data");
            if ( !TJA_DATA.UpdateSamples() ) Serial.println("       [TJA ERROR]  Handdle TJA packet loss");

            // Serial.print("Getting MPU data");
            if ( !MPU_DATA.UpdateSamples() ) Serial.println("       [MPU ERROR]  Handdle MPU sampling error");
        }
        vTaskDelay( xDelay );
    }
}

//  Toggle the medium frequency task state
void toggleMediumFreq( bool state ) {

    if ( state ) {
        sim_7000g.turnGPSOn();
        vTaskResume( xMediumFreq );
    } else {
        vTaskSuspend( xMediumFreq );
        sim_7000g.turnGPSOff();
    }
}

//  Sends the medium frequency data over mqtt
void TaskMediumFreq( void * parameters ){
    //  Suspend itself and wait to be awaken
    vTaskSuspend(NULL);

    static TickType_t xDelay = g_states.MQTTMediumPeriod / portTICK_PERIOD_MS;

    for (;;) {
        // Update only the MEDIUM frequency data as the high frequency data is already updated by this point
        if ( xSemaphoreTake( xModem, ( 250 / portTICK_PERIOD_MS ) ) == pdTRUE ){
            // Serial.print("Getting GPS data");
            if ( !sim_7000g.updateGPSData() ) Serial.println("        [GPS ERROR]");

            xSemaphoreGive( xModem );
        }

        if ( xSemaphoreTake( xModem, ( 250 / portTICK_PERIOD_MS ) ) == pdTRUE ){
            // Serial.print("Getting GPRS data");
            if ( !sim_7000g.update_GPRS_data() ) Serial.println("       [GPRS ERROR]");

            xSemaphoreGive( xModem );
        }

        vTaskDelay(xDelay);
    }
}

void reconnect () {

    //  Check if GPRS connection is close, if it is, re-opens
    // if ( !modem.isGprsConnected() ) sim_7000g.maintainGPRSconnection();
    Serial.print("GPRS connection");
    if ( !sim_7000g.checkConnection() ) {
        Serial.println("        [FALSE]");
        sim_7000g.maintainGPRSconnection();
    } else Serial.println("        [OK]");
    
    //  Check if MQTT connection is close, if it is, re-opens
    Serial.print("MQTT connection");
    if ( !mqtt.connected() ) {
        Serial.println("        [FALSE]");
        mqtt_com.maintainMQTTConnection();
    } else Serial.println("        [OK]");

}

//  Toggle the MQTT delivery task state
void toggleMQTTFreq( bool state ) {
    ( state )?vTaskResume( xMQTTDeliver ):vTaskSuspend( xMQTTDeliver );
}

// Task that sends data over MQTT
void taskSendOverMQTT ( void * parameters ) {
    //  Suspend itself and wait to be awaken
    vTaskSuspend(NULL);

    static TickType_t xHighDelay = g_states.MQTTHighPeriod / portTICK_PERIOD_MS;
    static TickType_t xMediumDelay = g_states.MQTTMediumPeriod / portTICK_PERIOD_MS;
    static TickType_t xErrDelay = g_states.MQTTCANErrPeriod / portTICK_PERIOD_MS;

    TickType_t itsHighTime = xTaskGetTickCount();
    TickType_t itsMediumTime = xTaskGetTickCount();
    TickType_t itsCANErrTime = xTaskGetTickCount();

    for (;;) {

        if ( xTaskGetTickCount() - itsHighTime > xHighDelay ) {
            if ( xSemaphoreTake( xModem, ( 200 / portTICK_PERIOD_MS ) ) == pdTRUE ) {
                // Send high frequency data through mqtt
                if ( !highFrequency.sendMHighFrequencyDataOverMQTT() ) Serial.println("     [HIGH ERROR]");
                itsHighTime = xTaskGetTickCount();

                xSemaphoreGive( xModem );
            }
        }

        vTaskDelay( 10 / portTICK_PERIOD_MS );

        if ( xTaskGetTickCount() - itsMediumTime > xMediumDelay ) {
            if ( xSemaphoreTake( xModem, ( 200 / portTICK_PERIOD_MS ) ) == pdTRUE ) {
                // Send medium frequency data through mqtt
                if ( !mediumFrequency.sendMediumFrequencyDataOverMQTT() ) Serial.println("     [MEDIUM ERROR]");

                itsMediumTime = xTaskGetTickCount();

                xSemaphoreGive( xModem );
            }
        }

        vTaskDelay( 10 / portTICK_PERIOD_MS );

        if ( xTaskGetTickCount() - itsCANErrTime > xErrDelay ) {
            if ( xSemaphoreTake( xModem, ( 200 / portTICK_PERIOD_MS ) ) == pdTRUE ) {
                // Send CAN error data through mqtt
                if ( !canErrorFrequency.sendCANErrorDataOverMQTT() ) Serial.println("     [CAN_ERR ERROR]");

                itsCANErrTime = xTaskGetTickCount();

                xSemaphoreGive( xModem );
            }
        }

        vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
}

//  Watch and logs for vehicle state warnings
void TaskWatchers ( void * parameters ) {

    //  All watchers methods
    for(;;){
        if ( !WatcherCurrent.isSetedUp ){
            // Sets up the watchers class
            if ( xSemaphoreTake( xSD, portMAX_DELAY ) == pdTRUE) {
                WatcherCurrent.setup();
                WatcherCurrent.isSetedUp = 1;
                xSemaphoreGive(xSD);
            }
        } else {
            if ( xSemaphoreTake( xSD, portMAX_DELAY ) == pdTRUE ){
                WatcherCurrent.current();
                xSemaphoreGive(xSD);
            }
        }
        vTaskDelay( 3000 / portTICK_PERIOD_MS );
    }
}

//  Task that Logs the data
void TasklogData ( void * parameters ) {
    // Suspend itself and wait to be awaken
    vTaskSuspend( NULL );

    for (;;) {
        if ( xSemaphoreTake( xSD, portMAX_DELAY ) == pdTRUE ) {
            SDlogger.updateFile();
            xSemaphoreGive(xSD);
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

//  Task that update board timestamp
void taskUpdateTimesTamp( void * parameters ){
    for (;;){
        if ( xSemaphoreTake( xModem, portMAX_DELAY ) == pdTRUE ) {
            sim_7000g.updateDateTime();
            xSemaphoreGive( xModem );
        }
        vTaskDelay( (750) / portTICK_PERIOD_MS );
    }
}

//  Task to handle the MQTT subscribe loop
void taskLoopClientMQTT ( void * parameters ) {
    char wakeTopic[sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTWakeTopic) + 3];
    sprintf( wakeTopic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTWakeTopic );
    TickType_t maintainConnectionUp_Time = xTaskGetTickCount();
    TickType_t updateTimestamp = xTaskGetTickCount();

    for(;;){

        if ( xSemaphoreTake( xModem, portMAX_DELAY ) == pdTRUE ) {
            mqtt.loop();

            //  Prevent from Idle disconnection
            if ( xTaskGetTickCount() - maintainConnectionUp_Time > (7500 / portTICK_PERIOD_MS) ){
                if ( mqtt.publish( wakeTopic, "I'm up..." ) ){
                    sim_7000g.last_message = xTaskGetTickCount();
                } else {
                    reconnect();
                    sim_7000g.last_message = xTaskGetTickCount();
                }
                maintainConnectionUp_Time = xTaskGetTickCount();
            }

            xSemaphoreGive( xModem );
        }

        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

void taskConnectionStatus ( void * parameters ) {
    for(;;){
       if ( sim_7000g.last_message > 0 ){
            if ( wb.isWissleblowingActive ){
                if ( (xTaskGetTickCount() - sim_7000g.last_message) * portTICK_PERIOD_MS > 10000) reconnect();
            } else {
                if ( (xTaskGetTickCount() - sim_7000g.last_message) * portTICK_PERIOD_MS > 30000) reconnect();
            }
            sim_7000g.last_message = xTaskGetTickCount();
       }
       vTaskDelay( 5000 );
    }
}

//  Client's heartbeat detection service task
void taskWebClientObserver ( void * parameters ) {
    //  Suspends itself and awaits for been awaken
    vTaskSuspend(NULL);

    for (;;) {
        //  If the Wissleblowing service is active but client doeasn't signal a hartbeat withing 45s,
        //  them suspend the wissleblowing service
        if ( wb.isWissleblowingActive ) {
            if ( (xTaskGetTickCount() - mqtt_com.subscribedLastHeartBeat) > 60000 / portTICK_PERIOD_MS ) {
                
                //  Flags that whissleblowing service is now inactive
                wb.stopWhistleblowing();

                Serial.print("Suspending the message system due to client's timeout");

                // Suspend the high frequency task
                toggleHighFreq( false );

                //  Suspend the medium frequency task
                toggleMediumFreq( false );

                //  Waits for the MQTTDeliveryTask to free the mutex before suspends it
                if ( xSemaphoreTake( xModem, portMAX_DELAY ) == pdTRUE ) {
                    //  Suspend the MQTT delivery task
                    toggleMQTTFreq( false );
                    xSemaphoreGive( xModem );
                }

                Serial.println("        [OK]");

                //  Suspend itself
                vTaskSuspend(NULL);
            }
        }

        vTaskDelay( 5000 / portTICK_PERIOD_MS );
    }
}

//  Task to bring the ESPServer down on timeout
void TaskESPServerObserver ( void * parameters ) {
    //  Suspends itself and await the Server to be up again
    vTaskSuspend(NULL);
    
    for(;;){
        ESPServer.ServerTimeout();
        vTaskDelay( 5000 / portTICK_PERIOD_MS );
    };
}

//  Task to hartbeat by buildint LED
void heartBeat ( void * parameters ) {
    digitalWrite( LED_PIN, HIGH );

    for(;;){
        //  Blinks the Hardware LED heartbeat 
        if ( digitalRead( LED_PIN ) ){
            digitalWrite( LED_PIN, LOW );
            vTaskDelay( 100 / portTICK_PERIOD_MS );
        } else {
            digitalWrite( LED_PIN, HIGH );
            vTaskDelay( 1000 / portTICK_PERIOD_MS );
        };

        vTaskDelay( 50 / portTICK_PERIOD_MS );
    }
}

//  Task to listen voltage source removal
void vontageInput ( void * parameter ) {
    for (;;){
        uint16_t voltageValue = analogRead( ESP_BAT_PIN );
        // Serial.println(voltageValue);

        if ( (voltageValue == 4095 || voltageValue == 0) && !g_states.VoltageInput ){
            const char message[] = "Device connected to external voltage source";
            char topic_Wake[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTListenTopic) + 3 ];
            sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTListenTopic );

            Serial.println(message);
            if ( mqtt.publish(topic_Wake, message) ){ sim_7000g.last_message = xTaskGetTickCount(); };

            //  Sets up the MPU6050 class
            MPU_DATA.setup();
            //  Sets up the TJA1050 class
            TJA_DATA.setup();

            g_states.VoltageInput = true;

        } else if ( (voltageValue < 4095 && voltageValue > 0) && g_states.VoltageInput ) {
            const char message[] = "Device disconnected from external voltage source";
            char topic_Wake[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTListenTopic) + 3 ];
            sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTListenTopic );

            Serial.println(message);
            if (mqtt.publish(topic_Wake, message)){ sim_7000g.last_message = xTaskGetTickCount(); };

            g_states.VoltageInput = false;
        };

        vTaskDelay( 500 / portTICK_PERIOD_MS );
    }
}

void setup() {

    gpio_set_level(ESP_RST_PIN, 1);
    gpio_set_direction(ESP_RST_PIN, GPIO_MODE_OUTPUT);

    if ( !Serial ) Serial.begin(115200);
    while( !Serial );

    //  Clear flash memory and reset
    utilities.ESPClearFlash();

    analogSetWidth(12);
    analogSetPinAttenuation(ESP_BAT_PIN, ADC_11db);
    
    pinMode(LED_PIN, OUTPUT);

    // Load the configdata saved on SPIFFS
    g_states.loadConfigFromSPIFFS();

    //  Handle crashed esp reboot loop
    AntiCrash.checkLastCrash();

    xModem = xSemaphoreCreateMutex();
    xSD = xSemaphoreCreateMutex();

    //  Sets up the datalloger
    sd_card.setup();

    // Prints the current CPU frequency
    uint32_t freq = getCpuFrequencyMhz();
    Serial.print("Current CPU frequency: ");
    Serial.print(freq);
    Serial.println(" MHz");

    // Sets up the SIM7000G GPRS class
    sim_7000g.setup();
    // Sets up the MQTT class
    mqtt_com.setup();
    // Sets up the OTA class
    OTA.setup();

    //  Sets up the MPU6050 class
    MPU_DATA.setup();
    //  Sets up the TJA1050 class
    TJA_DATA.setup();

    // //  Sendo good morning message
    char topic_Wake[ sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTWakeTopic) + 3 ];
    sprintf( topic_Wake, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTWakeTopic );
    if (mqtt.publish( topic_Wake, "Just woke. Good morning!" )){ sim_7000g.last_message = xTaskGetTickCount(); }

    xTaskCreatePinnedToCore( TaskMediumFreq, "Medium frequency data task", 4096, NULL, 2, &xMediumFreq, 0 );
    xTaskCreatePinnedToCore( TaskHighFreq, "High frequency data task", 2048, NULL, 2, &xHighFreq, 0 );
    // xTaskCreatePinnedToCore( TaskWatchers, "Watcher for state warnings", 2048, NULL, 3, NULL, 0 );
    xTaskCreatePinnedToCore( TasklogData, "Logs data to SD card", 5120, NULL, 3, &xLog, 0 );

    xTaskCreatePinnedToCore( taskSendOverMQTT, "MQTT data delivery task", 5120, NULL, 0, &xMQTTDeliver, 1 );
    xTaskCreatePinnedToCore( taskLoopClientMQTT, "MQTT client loop", 9216, NULL, 1, &xMQTTLoop, 1 );
    xTaskCreatePinnedToCore( TaskESPServerObserver, "Observe if ESPServer timesout", 2048, NULL, 1, &xESPServer, 1 );
    // xTaskCreatePinnedToCore( vontageInput, "Observe voltage input removal", 4096, NULL, 1, &xVoltageIn, 1 );

    xTaskCreatePinnedToCore( taskUpdateTimesTamp, "Update board's Timestamp", 2048, NULL, 4, &xRTC, 0 );
    xTaskCreatePinnedToCore( heartBeat, "Blinks the LED", 1024, NULL, 1, &xHeartBeat, 0 );
    xTaskCreatePinnedToCore( taskWebClientObserver, "Watch for web client's timeout", 1024, NULL, 1, &xClientsHeartbeat, 0 );
    xTaskCreatePinnedToCore( taskConnectionStatus, "Check for internet connection", 2048, NULL, 1, NULL, 0 );
    

    vTaskDelay( 200 / portTICK_PERIOD_MS );

    //  Check for previous runtime events
    SDlogger.autoStartLog();
    wb.autoStartWhistleblowing();

    
}

//  We'll not need this task, so we delete it
void loop(){ vTaskDelete(NULL); };