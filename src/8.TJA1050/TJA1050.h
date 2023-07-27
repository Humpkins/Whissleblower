#include "Arduino.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/can.h"

// #define DEBUG_CAN
#define SET_CAN_FILTER

#define CAN_RX GPIO_NUM_32
#define CAN_TX GPIO_NUM_25

#define N_BATTERIES 2

class TJA1050 {
    private:
        bool anyFalse ( bool * boolArray ) {
            //  Check if at least one battery send info and error data
            return ( boolArray[0] || boolArray[1] ) && (boolArray[4] || boolArray[5]) && boolArray[2] && boolArray[3];
        }

    public:

        // Creates a struct to store bateries medium frequency info
        struct batteryInfo {
            int current = 0;
            int voltage = 0;
            int SoC = 0;
            int SoH = 0;
            int temperature = 0;
            int capacity = 0;
        };
        // Instantiate an array with all baterries configured
        struct batteryInfo batteries[N_BATTERIES];

        // Creates a struct to store powertrain's data
        struct powertrainInfo {
            int motorSpeedRPM = 0;
            int motorTorque = 0;
            int motorTemperature = 0;
            int controllerTemperature = 0;
        };
        // Instantiate the powertrain structure
        struct powertrainInfo CurrentPowertrainData;

        // Creates a struct to store MCU's error log
        struct ControllerErrorInfo {
            int hardwareFault1 = 0;                         // hardware fault
            int motorSensor = 0;                            // motor sensor error
            int overVoltage = 0;                            // over voltage
            int underVoltage = 0;                           // under voltage 
            int overTemperature = 0;                        // over temperatue
            int overCurrent = 0;                            // over current
            int overLoad = 0;                               // over load
            int motorLock = 0;                              // motor lock protection
            int hardwareFault2 = 0;                         // hardware fault
            int hardwareFault3 = 0;                         // hardware fault
            int motorSensorNotConnected = 0;                // motor sensor not connect
            int hardwareFault4 = 0;                         // hardware fault
            int hardwareFault5 = 0;                         // hardware fault
            int motorTempSensShort = 0;                     // motor temperature sensor short
            int motorTempSensOpen = 0;                      // motor temperature sensor open
        };
        //  Instantiate the err structure
        struct ControllerErrorInfo MCU_error;

        // Creates a struct to store BMS error data
        struct BMSErrorInfo {
            int W_cell_chg = 0;                              // cell over charge warning
            int E_cell_chg = 0;                              // cell over charge error
            int W_pkg_overheat = 0;                          // pack charge over heat warning
            int E_pkg_chg_overheat = 0;                      // pack charge over heat error
            int W_pkg_chg_undertemp = 0;                     // pack charge low temperatue warning
            int E_pkg_chg_undertemp = 0;                     // pack charge low temperatue error
            int W_pkg_chg_overcurrent = 0;                   // pack chare over current warning
            int E_pkg_chg_overcurrent = 0;                   // pack chare over current error
            int W_pkg_overvoltage = 0;                       // pack over voltage warning
            int E_pkg_overvoltage = 0;                       // pack over voltage error
            int E_charger_COM = 0;                           // communication error with charger
            int E_pkg_chg_softstart = 0;                     // pack charge soft start error
            int E_chg_relay_stuck = 0;                       // charging relay stuck
            int W_cell_dchg_undervoltage = 0;                // cell discharge under voltage warning
            int E_cell_dchg_undervoltage = 0;                // cell discharge under voltage error
            int E_cell_deep_undervoltage = 0;                // cell deep under voltage
            int W_pkg_dchg_overheat = 0;                     // pack discharge over heat warning
            int E_pkg_dchg_overheat = 0;                     // pack discharge over heat error
            int W_pkg_dchg_undertemp = 0;                    // discharge low temperature warning
            int E_pkg_dchg_undertemp = 0;                    // pack discharge low temperature error
            int W_pkg_dchg_overcurrent = 0;                  // pack dischage over current waning
            int E_pkg_dchg_overcurrent = 0;                  // pack dischage over current error
            int W_pkg_undervoltage = 0;                      // pack under voltage warning
            int E_pkg_undervoltage = 0;                      // pack under voltage  error
            int E_VCU_COM = 0;                               // Communication error to VCU
            int E_pkg_dchg_softstart = 0;                    // pack discharge soft start error
            int E_dchg_relay_stuck = 0;                      // discharging relay stuck
            int E_pkg_dchg_short = 0;                        // pack discharge short
            int E_pkg_temp_diff = 0;                         // pack excessive temperature differentials
            int E_cell_voltage_diff = 0;                     // cell excessive voltage differentials
            int E_AFE = 0;                                   // AFE Error
            int E_MOS_overtemp = 0;                          // MOS over temperature
            int E_external_EEPROM = 0;                       // external EEPROM failure
            int E_RTC = 0;                                   // RTC failure
            int E_ID_conflict = 0;                           // ID conflict
            int E_CAN_msg_miss = 0;                          // CAN message miss
            int E_pkg_voltage_diff = 0;                      // pack excessive voltage differentials
            int E_chg_dchg_current_conflict = 0;             // charge and discharge current conflict
            int E_cable_abnormal = 0;                        // cable abnormal
        };
        //  Instantiate the err structure
        struct BMSErrorInfo BMS_Error[N_BATTERIES];

        void setup(){

            // Configure the TJA1050 for CAN communication
            can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, CAN_MODE_NORMAL);
            can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();    
            can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

            #ifdef SET_CAN_FILTER

                f_config.single_filter = false;
                
                f_config.acceptance_code = 0x24006000;
                f_config.acceptance_mask = 0x023F003F;
            #endif

            ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
            ESP_ERROR_CHECK(can_start());

        }

        //  Function that update the IDs data from the incoming CAN packets
        bool UpdateSamples() {

            // Loop control variables
            bool already_checked[6] = { false, false, false, false, false, false };  //  IDs already checked

            const TickType_t tickLimit = xTaskGetTickCount() + (g_states.MQTTMediumPeriod / portTICK_PERIOD_MS) ; // Timeout limit

            while ( this->anyFalse( already_checked ) == false ) {
                can_message_t message;

                if (can_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {

                    int16_t packetId = message.identifier;
                    int16_t packetSize = message.data_length_code;
                    int16_t packetData[ packetSize ];
                    for ( int i = 0; i < packetSize; i++ ) {
                        packetData[ i ] = message.data[i];
                    }

                    #ifdef DEBUG_CAN
                        //  Print the received data
                        if ( !Serial ) Serial.begin(115200);
                        printf( "0x%08x ", packetId );
                        for ( int i = 0; i < packetSize; i++ ) {
                            printf("%02x ", packetData[i]);
                        }
                        printf("\n");
                    #endif

                    if ( (packetId == g_states.BASE_BATTERY_ID && !already_checked[0]) || (packetId == g_states.BASE_BATTERY_ID + 1 && !already_checked[1]) ) {
                        
                        int16_t index = 0;
                        if ( packetId == g_states.BASE_BATTERY_ID ) { index = 0; }
                        else { index = 1; }

                        // Read the BMS1 Data and save it to the gloabl state
                        this->batteries[index].current        = (packetData[2] * (int)pow(16, 2) + packetData[3]) * 0.1;
                        this->batteries[index].voltage        = (packetData[0] * (int)pow(16, 2) + packetData[1]) * 0.1; // deslocamento para a esqueda
                        this->batteries[index].SoC            = packetData[6];
                        this->batteries[index].SoH            = packetData[7];
                        this->batteries[index].temperature    = packetData[4];

                        // Flag this packet as already checked
                        already_checked[index] = true;

                    } else if ( packetId == g_states.BASE_CONTROLLER_ID && !already_checked[2] ) {

                        // Read the controller data and save it to the gloabl state
                        this->CurrentPowertrainData.motorSpeedRPM         = packetData[0] * (int)pow(16, 2) + packetData[1];
                        this->CurrentPowertrainData.motorTorque           = (packetData[2] * (int)pow(16, 2) + packetData[3]) * 0.1;
                        this->CurrentPowertrainData.motorTemperature      = packetData[7] - 40;
                        this->CurrentPowertrainData.controllerTemperature = packetData[6] - 40;

                        // Flag this packet as already checked
                        already_checked[2] = true;

                    } else if ( packetId == g_states.BASE_CONTROLLER_ID_2 && !already_checked[3] ) {

                        this->MCU_error.hardwareFault1 = ( packetData[2] & (0x80 >> 0) ) >> (7 - 0);
                        this->MCU_error.motorSensor = ( packetData[2] & (0x80 >> 1) ) >> (7 - 1);
                        this->MCU_error.overVoltage = ( packetData[2] & (0x80 >> 2) ) >> (7 - 2);
                        this->MCU_error.underVoltage = ( packetData[2] & (0x80 >> 3) ) >> (7 - 3);
                        this->MCU_error.overTemperature = ( packetData[2] & (0x80 >> 4) ) >> (7 - 4);
                        this->MCU_error.overCurrent = ( packetData[2] & (0x80 >> 5) ) >> (7 - 5);
                        this->MCU_error.overLoad = ( packetData[2] & (0x80 >> 6) ) >> (7 - 6);
                        this->MCU_error.motorLock = ( packetData[2] & (0x80 >> 7) ) >> (7 - 7);
                        this->MCU_error.hardwareFault2 = ( packetData[3] & (0x80 >> 0) ) >> (7 - 0);
                        this->MCU_error.hardwareFault3 = ( packetData[3] & (0x80 >> 1) ) >> (7 - 1);
                        this->MCU_error.motorSensorNotConnected = ( packetData[3] & (0x80 >> 2) ) >> (7 - 2);
                        this->MCU_error.hardwareFault4 = ( packetData[3] & (0x80 >> 3) ) >> (7 - 3);
                        this->MCU_error.hardwareFault5 = ( packetData[3] & (0x80 >> 4) ) >> (7 - 4);
                        this->MCU_error.motorTempSensShort = ( packetData[3] & (0x80 >> 5) ) >> (7 - 5);
                        this->MCU_error.motorTempSensOpen = ( packetData[3] & (0x80 >> 6) ) >> (7 - 6);

                    } else if ( (packetId == g_states.BASE_BATTERY_ID_2 && !already_checked[4]) || (packetId == g_states.BASE_BATTERY_ID_2 + 1 && !already_checked[5]) ) {
                        
                        int16_t index = 0;
                        int16_t idOffset = 4;
                        if ( packetId == g_states.BASE_BATTERY_ID_2 ) { index = 0; }
                        else { index = 1; }

                        this->BMS_Error[index].W_cell_chg =                        ( packetData[0] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].E_cell_chg =                        ( packetData[0] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].W_pkg_overheat =                    ( packetData[0] & (0x80 >> 2) ) >> (7 - 2);
                        this->BMS_Error[index].E_pkg_chg_overheat =                ( packetData[0] & (0x80 >> 3) ) >> (7 - 3);
                        this->BMS_Error[index].W_pkg_chg_undertemp =               ( packetData[0] & (0x80 >> 4) ) >> (7 - 4);
                        this->BMS_Error[index].E_pkg_chg_undertemp =               ( packetData[0] & (0x80 >> 5) ) >> (7 - 5);
                        this->BMS_Error[index].W_pkg_chg_overcurrent =             ( packetData[0] & (0x80 >> 6) ) >> (7 - 6);
                        this->BMS_Error[index].E_pkg_chg_overcurrent =             ( packetData[0] & (0x80 >> 7) ) >> (7 - 7);
                        this->BMS_Error[index].W_pkg_overvoltage =                 ( packetData[1] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].E_pkg_overvoltage =                 ( packetData[1] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].E_charger_COM =                     ( packetData[1] & (0x80 >> 2) ) >> (7 - 2);
                        this->BMS_Error[index].E_pkg_chg_softstart =               ( packetData[1] & (0x80 >> 3) ) >> (7 - 3);
                        this->BMS_Error[index].E_chg_relay_stuck =                 ( packetData[1] & (0x80 >> 4) ) >> (7 - 4);
                        this->BMS_Error[index].W_cell_dchg_undervoltage =          ( packetData[2] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].E_cell_dchg_undervoltage =          ( packetData[2] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].E_cell_deep_undervoltage =          ( packetData[2] & (0x80 >> 2) ) >> (7 - 2);
                        this->BMS_Error[index].W_pkg_dchg_overheat =               ( packetData[2] & (0x80 >> 3) ) >> (7 - 3);
                        this->BMS_Error[index].E_pkg_dchg_overheat =               ( packetData[2] & (0x80 >> 4) ) >> (7 - 4);
                        this->BMS_Error[index].W_pkg_dchg_undertemp =              ( packetData[2] & (0x80 >> 5) ) >> (7 - 5);
                        this->BMS_Error[index].E_pkg_dchg_undertemp =              ( packetData[2] & (0x80 >> 6) ) >> (7 - 6);
                        this->BMS_Error[index].W_pkg_dchg_overcurrent =            ( packetData[2] & (0x80 >> 7) ) >> (7 - 7);
                        this->BMS_Error[index].E_pkg_dchg_overcurrent =            ( packetData[3] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].W_pkg_undervoltage =                ( packetData[3] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].E_pkg_undervoltage =                ( packetData[3] & (0x80 >> 2) ) >> (7 - 2);
                        this->BMS_Error[index].E_VCU_COM =                         ( packetData[3] & (0x80 >> 3) ) >> (7 - 3);
                        this->BMS_Error[index].E_pkg_dchg_softstart =              ( packetData[3] & (0x80 >> 4) ) >> (7 - 4);
                        this->BMS_Error[index].E_dchg_relay_stuck =                ( packetData[3] & (0x80 >> 5) ) >> (7 - 5);
                        this->BMS_Error[index].E_pkg_dchg_short =                  ( packetData[3] & (0x80 >> 6) ) >> (7 - 6);
                        this->BMS_Error[index].E_pkg_temp_diff =                   ( packetData[6] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].E_cell_voltage_diff =               ( packetData[6] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].E_AFE =                             ( packetData[6] & (0x80 >> 2) ) >> (7 - 2);
                        this->BMS_Error[index].E_MOS_overtemp =                    ( packetData[6] & (0x80 >> 3) ) >> (7 - 3);
                        this->BMS_Error[index].E_external_EEPROM =                 ( packetData[6] & (0x80 >> 4) ) >> (7 - 4);
                        this->BMS_Error[index].E_RTC =                             ( packetData[6] & (0x80 >> 5) ) >> (7 - 5);
                        this->BMS_Error[index].E_ID_conflict =                     ( packetData[6] & (0x80 >> 6) ) >> (7 - 6);
                        this->BMS_Error[index].E_CAN_msg_miss =                    ( packetData[6] & (0x80 >> 7) ) >> (7 - 7);
                        this->BMS_Error[index].E_pkg_voltage_diff =                ( packetData[7] & (0x80 >> 0) ) >> (7 - 0);
                        this->BMS_Error[index].E_chg_dchg_current_conflict =       ( packetData[7] & (0x80 >> 1) ) >> (7 - 1);
                        this->BMS_Error[index].E_cable_abnormal =                  ( packetData[7] & (0x80 >> 2) ) >> (7 - 2);

                        // Flag this packet as already checked
                        already_checked[index + idOffset] = true;
                    }
                    
                }

                // Timeout limit
                if ( xTaskGetTickCount() >= tickLimit ) return false;

            }

            // If all the packets were received, return true
            return true;
        }
};

TJA1050 TJA_DATA;