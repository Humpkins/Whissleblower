// #define LOGGER_DEBUG

class LOGGER {
    public:
        char basePath[15] = "logger_";
        char constPath[40];
        char stateFile[24] = "/wasRecording.txt";
        FILE * logFile;

        int16_t updateAttemptCount = 0;
        const int16_t updateAttemptMax = 5;

        bool recording = false;

        /* Stop recording service */
        void stopLog(){

            //  Stop logging if it is doing it   
            if ( this->recording ) {
                if ( this->logFile != NULL ) fclose( this->logFile );
                this->logFile = NULL;

                //  Flags not-recording state
                this->recording = false;
                #ifdef SPIFFS_H
                SPIFFS.begin();
                File recordingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
                recordingFile.write('0');
                recordingFile.close();
                SPIFFS.end();
                #endif

                vTaskSuspend( xLog );

            }

            return;
        }

        #ifdef SPIFFS_H
        /* Auto start recording service
        This method aims to restart the recording service if it was suddenly interrupted  on the
        previous runtime
        0 - Failed
        1 - Success*/
        int autoStartLog(){

            Serial.print("\nCheck if it was Recording data on the last runtime");

            SPIFFS.begin();
            File recordingFile = SPIFFS.open(this->stateFile, FILE_READ);

            if ( !recordingFile ) {
                Serial.println("      [FAIL]");
                Serial.println("Last recording state file does not exist. Creating one and continuing the program");

                recordingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
                recordingFile.write('0');
                recordingFile.close();
                SPIFFS.end();

                return 0;
            }

            int previousRecordingStateLen = 1;
            char previousRecordingState[ previousRecordingStateLen + 1 ];
            int loop_read = 0;

            while ( recordingFile.available() && loop_read < previousRecordingStateLen ) {
                previousRecordingState[ loop_read ] = recordingFile.read();
                loop_read++;
            }
            previousRecordingState[ loop_read ] = '\0';

            recordingFile.close();

            if ( strcmp( previousRecordingState, "1" ) != 0 ) {
                Serial.println("      [OK]");
                Serial.println("It was not recording on the previous runtime. Continuing the program");
                SPIFFS.end();
                return 0;
            } else {
                Serial.println("      [OK]");
                Serial.println("Resuming the recording service started on the previous runtime");
                this->startLog();
                return 1;
            }

        };
        #endif

        /*  0 - Fail to mount SD
            1 - Successefully started recording
            2 - Loggin service is already running
            3 - Filed to open log file */
        int startLog(){
            
            //  Starts logging if it is not already doing it
            if ( !this->recording ) {

                char * sanitized = utilities.replace_char( sim_7000g.CurrentGPSData.datetime, '/', '_' );
                sanitized = utilities.replace_char( sim_7000g.CurrentGPSData.datetime, ':', '-' );

                sprintf( this->constPath, "%s/%s%s.txt", MOUNT_POINT, this->basePath, sanitized );
                Serial.printf("Creating file: %s", this->constPath);

                if ( this->logFile == NULL ) {
                    this->logFile = fopen(this->constPath, FILE_WRITE);
                    if ( this->logFile == NULL ) {
                        Serial.println("        [FAIL]");
                        return 3;
                    } else Serial.println("     [OK]");
                    vTaskDelay( 1000 / portTICK_PERIOD_MS );
                }

                Serial.end();
 
                char fileHeader[2296];
                strcpy(fileHeader, "Timestamp,current1,current2,voltage1,voltage2,SoC1,SoC2,SoH1,SoH2,Temp1,Temp2,motorTemp,controllerTemp,speed,torque,gprsQlty,GPRSOpMode,inViewGNSS,usedGPS,usedGLONASS,Lat,Lon,gps_speed,gps_altitude,MCC,MNC,LAC,CellID,pitch,yall,roll,acc_x,acc_y,acc_z,hardwareFault1, motorSensor, overVoltage, underVoltage, overTemperature, overCurrent, overLoad, motorLock, hardwareFault2, hardwareFault3, motorSensorNotConnected, hardwareFault4, hardwareFault5, motorTempSensShort, motorTempSensOpen,BMS1_W_cell_chg,BMS1_E_cell_chg,BMS1_W_pkg_overheat,BMS1_E_pkg_chg_overheat,BMS1_W_pkg_chg_undertemp,BMS1_E_pkg_chg_undertemp,BMS1_W_pkg_chg_overcurrent,BMS1_E_pkg_chg_overcurrent,BMS1_W_pkg_overvoltage,BMS1_E_pkg_overvoltage,BMS1_E_charger_COM,BMS1_E_pkg_chg_softstart,BMS1_E_chg_relay_stuck,BMS1_W_cell_dchg_undervoltage,BMS1_E_cell_dchg_undervoltage,BMS1_E_cell_deep_undervoltage,BMS1_W_pkg_dchg_overheat,BMS1_E_pkg_dchg_overheat,BMS1_W_pkg_dchg_undertemp,BMS1_E_pkg_dchg_undertemp,BMS1_W_pkg_dchg_overcurrent,BMS1_E_pkg_dchg_overcurrent,BMS1_W_pkg_undervoltage,BMS1_E_pkg_undervoltage,BMS1_E_VCU_COM,BMS1_E_pkg_dchg_softstart,BMS1_E_dchg_relay_stuck,BMS1_E_pkg_dchg_short,BMS1_E_pkg_temp_diff,BMS1_E_cell_voltage_diff,BMS1_E_AFE,BMS1_E_MOS_overtemp,BMS1_E_external_EEPROM,BMS1_E_RTC,BMS1_E_ID_conflict,BMS1_E_CAN_msg_miss,BMS1_E_pkg_voltage_diff,BMS1_E_chg_dchg_current_conflict,BMS1_E_cable_abnormal,BMS2_W_cell_chg,BMS2_E_cell_chg,BMS2_W_pkg_overheat,BMS2_E_pkg_chg_overheat,BMS2_W_pkg_chg_undertemp,BMS2_E_pkg_chg_undertemp,BMS2_W_pkg_chg_overcurrent,BMS2_E_pkg_chg_overcurrent,BMS2_W_pkg_overvoltage,BMS2_E_pkg_overvoltage,BMS2_E_charger_COM,BMS2_E_pkg_chg_softstart,BMS2_E_chg_relay_stuck,BMS2_W_cell_dchg_undervoltage,BMS2_E_cell_dchg_undervoltage,BMS2_E_cell_deep_undervoltage,BMS2_W_pkg_dchg_overheat,BMS2_E_pkg_dchg_overheat,BMS2_W_pkg_dchg_undertemp,BMS2_E_pkg_dchg_undertemp,BMS2_W_pkg_dchg_overcurrent,BMS2_E_pkg_dchg_overcurrent,BMS2_W_pkg_undervoltage,BMS2_E_pkg_undervoltage,BMS2_E_VCU_COM,BMS2_E_pkg_dchg_softstart,BMS2_E_dchg_relay_stuck,BMS2_E_pkg_dchg_short,BMS2_E_pkg_temp_diff,BMS2_E_cell_voltage_diff,BMS2_E_AFE,BMS2_E_MOS_overtemp,BMS2_E_external_EEPROM,BMS2_E_RTC,BMS2_E_ID_conflict,BMS2_E_CAN_msg_miss,BMS2_E_pkg_voltage_diff,BMS2_E_chg_dchg_current_conflict,BMS2_E_cable_abnormal\n");
                // 34 fields
                if ( fprintf( this->logFile, fileHeader ) < 0 ){
                    //  Bring Serial COM back on
                    Serial.begin(115200);
                    while( !Serial );

                    Serial.println("        [FAIL]");
                    Serial.println("It wasn't possible to write the csv header");

                    return 3;
                }

                fclose(this->logFile);
                this->logFile = NULL;

                //  Flags recording state
                this->recording = true;
                SPIFFS.begin();
                File recordingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
                recordingFile.write('1');
                recordingFile.close();
                SPIFFS.end();

                this->updateAttemptCount = 0;

                //  Resume the SD writing task
                vTaskResume( xLog );

                //  Bring Serial COM back on
                Serial.begin(115200);
                while( !Serial );

                //  Return success on SD mount attempt
                return 1;

            } else return 2;
        }

        void updateFile(){

            this->logFile = fopen(this->constPath, FILE_APPEND);

            char dataToBeSaved[500];
            sprintf(
                dataToBeSaved,
                "%s, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %s, %i, %i, %i, %.8f, %.8f, %1.f, %.1f, %i, %i, %x, %i, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i\n",
                sim_7000g.CurrentGPSData.datetime,
                TJA_DATA.batteries[0].current,
                TJA_DATA.batteries[1].current,
                TJA_DATA.batteries[0].voltage,
                TJA_DATA.batteries[1].voltage,
                TJA_DATA.batteries[0].SoC,
                TJA_DATA.batteries[1].SoC,
                TJA_DATA.batteries[0].SoH,
                TJA_DATA.batteries[1].SoH,
                TJA_DATA.batteries[0].temperature,
                TJA_DATA.batteries[1].temperature,
                TJA_DATA.CurrentPowertrainData.motorTemperature,
                TJA_DATA.CurrentPowertrainData.controllerTemperature,
                TJA_DATA.CurrentPowertrainData.motorSpeedRPM,
                TJA_DATA.CurrentPowertrainData.motorTorque,
                sim_7000g.CurrentGPRSData.signalQlty,
                sim_7000g.CurrentGPRSData.operationalMode,
                sim_7000g.CurrentGPSData.vSatGNSS,
                sim_7000g.CurrentGPSData.uSatGPS,
                sim_7000g.CurrentGPSData.uSatGLONASS,
                sim_7000g.CurrentGPSData.latitude,
                sim_7000g.CurrentGPSData.longitude,
                sim_7000g.CurrentGPSData.speed,
                sim_7000g.CurrentGPSData.altitude,
                sim_7000g.CurrentGPRSData.MCC,
                sim_7000g.CurrentGPRSData.MNC,
                sim_7000g.CurrentGPRSData.LAC,
                sim_7000g.CurrentGPRSData.cellID,
                MPU_DATA.MPUData.ypr[1],
                MPU_DATA.MPUData.ypr[0],
                MPU_DATA.MPUData.ypr[2],
                (float)MPU_DATA.MPUData.aaWorld.x,
                (float)MPU_DATA.MPUData.aaWorld.y,
                (float)MPU_DATA.MPUData.aaWorld.z,
                TJA_DATA.MCU_error.hardwareFault1,
                TJA_DATA.MCU_error.motorSensor,
                TJA_DATA.MCU_error.overVoltage,
                TJA_DATA.MCU_error.underVoltage,
                TJA_DATA.MCU_error.overTemperature,
                TJA_DATA.MCU_error.overCurrent,
                TJA_DATA.MCU_error.overLoad,
                TJA_DATA.MCU_error.motorLock,
                TJA_DATA.MCU_error.hardwareFault2,
                TJA_DATA.MCU_error.hardwareFault3,
                TJA_DATA.MCU_error.motorSensorNotConnected,
                TJA_DATA.MCU_error.hardwareFault4,
                TJA_DATA.MCU_error.hardwareFault5,
                TJA_DATA.MCU_error.motorTempSensShort,
                TJA_DATA.MCU_error.motorTempSensOpen,

                TJA_DATA.BMS_Error[0].W_cell_chg,
                TJA_DATA.BMS_Error[0].E_cell_chg,
                TJA_DATA.BMS_Error[0].W_pkg_overheat,
                TJA_DATA.BMS_Error[0].E_pkg_chg_overheat,
                TJA_DATA.BMS_Error[0].W_pkg_chg_undertemp,
                TJA_DATA.BMS_Error[0].E_pkg_chg_undertemp,
                TJA_DATA.BMS_Error[0].W_pkg_chg_overcurrent,
                TJA_DATA.BMS_Error[0].E_pkg_chg_overcurrent,
                TJA_DATA.BMS_Error[0].W_pkg_overvoltage,
                TJA_DATA.BMS_Error[0].E_pkg_overvoltage,
                TJA_DATA.BMS_Error[0].E_charger_COM,
                TJA_DATA.BMS_Error[0].E_pkg_chg_softstart,
                TJA_DATA.BMS_Error[0].E_chg_relay_stuck,
                TJA_DATA.BMS_Error[0].W_cell_dchg_undervoltage,
                TJA_DATA.BMS_Error[0].E_cell_dchg_undervoltage,
                TJA_DATA.BMS_Error[0].E_cell_deep_undervoltage,
                TJA_DATA.BMS_Error[0].W_pkg_dchg_overheat,
                TJA_DATA.BMS_Error[0].E_pkg_dchg_overheat,
                TJA_DATA.BMS_Error[0].W_pkg_dchg_undertemp,
                TJA_DATA.BMS_Error[0].E_pkg_dchg_undertemp,
                TJA_DATA.BMS_Error[0].W_pkg_dchg_overcurrent,
                TJA_DATA.BMS_Error[0].E_pkg_dchg_overcurrent,
                TJA_DATA.BMS_Error[0].W_pkg_undervoltage,
                TJA_DATA.BMS_Error[0].E_pkg_undervoltage,
                TJA_DATA.BMS_Error[0].E_VCU_COM,
                TJA_DATA.BMS_Error[0].E_pkg_dchg_softstart,
                TJA_DATA.BMS_Error[0].E_dchg_relay_stuck,
                TJA_DATA.BMS_Error[0].E_pkg_dchg_short,
                TJA_DATA.BMS_Error[0].E_pkg_temp_diff,
                TJA_DATA.BMS_Error[0].E_cell_voltage_diff,
                TJA_DATA.BMS_Error[0].E_AFE,
                TJA_DATA.BMS_Error[0].E_MOS_overtemp,
                TJA_DATA.BMS_Error[0].E_external_EEPROM,
                TJA_DATA.BMS_Error[0].E_RTC,
                TJA_DATA.BMS_Error[0].E_ID_conflict,
                TJA_DATA.BMS_Error[0].E_CAN_msg_miss,
                TJA_DATA.BMS_Error[0].E_pkg_voltage_diff,
                TJA_DATA.BMS_Error[0].E_chg_dchg_current_conflict,
                TJA_DATA.BMS_Error[0].E_cable_abnormal,

                TJA_DATA.BMS_Error[1].W_cell_chg,
                TJA_DATA.BMS_Error[1].E_cell_chg,
                TJA_DATA.BMS_Error[1].W_pkg_overheat,
                TJA_DATA.BMS_Error[1].E_pkg_chg_overheat,
                TJA_DATA.BMS_Error[1].W_pkg_chg_undertemp,
                TJA_DATA.BMS_Error[1].E_pkg_chg_undertemp,
                TJA_DATA.BMS_Error[1].W_pkg_chg_overcurrent,
                TJA_DATA.BMS_Error[1].E_pkg_chg_overcurrent,
                TJA_DATA.BMS_Error[1].W_pkg_overvoltage,
                TJA_DATA.BMS_Error[1].E_pkg_overvoltage,
                TJA_DATA.BMS_Error[1].E_charger_COM,
                TJA_DATA.BMS_Error[1].E_pkg_chg_softstart,
                TJA_DATA.BMS_Error[1].E_chg_relay_stuck,
                TJA_DATA.BMS_Error[1].W_cell_dchg_undervoltage,
                TJA_DATA.BMS_Error[1].E_cell_dchg_undervoltage,
                TJA_DATA.BMS_Error[1].E_cell_deep_undervoltage,
                TJA_DATA.BMS_Error[1].W_pkg_dchg_overheat,
                TJA_DATA.BMS_Error[1].E_pkg_dchg_overheat,
                TJA_DATA.BMS_Error[1].W_pkg_dchg_undertemp,
                TJA_DATA.BMS_Error[1].E_pkg_dchg_undertemp,
                TJA_DATA.BMS_Error[1].W_pkg_dchg_overcurrent,
                TJA_DATA.BMS_Error[1].E_pkg_dchg_overcurrent,
                TJA_DATA.BMS_Error[1].W_pkg_undervoltage,
                TJA_DATA.BMS_Error[1].E_pkg_undervoltage,
                TJA_DATA.BMS_Error[1].E_VCU_COM,
                TJA_DATA.BMS_Error[1].E_pkg_dchg_softstart,
                TJA_DATA.BMS_Error[1].E_dchg_relay_stuck,
                TJA_DATA.BMS_Error[1].E_pkg_dchg_short,
                TJA_DATA.BMS_Error[1].E_pkg_temp_diff,
                TJA_DATA.BMS_Error[1].E_cell_voltage_diff,
                TJA_DATA.BMS_Error[1].E_AFE,
                TJA_DATA.BMS_Error[1].E_MOS_overtemp,
                TJA_DATA.BMS_Error[1].E_external_EEPROM,
                TJA_DATA.BMS_Error[1].E_RTC,
                TJA_DATA.BMS_Error[1].E_ID_conflict,
                TJA_DATA.BMS_Error[1].E_CAN_msg_miss,
                TJA_DATA.BMS_Error[1].E_pkg_voltage_diff,
                TJA_DATA.BMS_Error[1].E_chg_dchg_current_conflict,
                TJA_DATA.BMS_Error[1].E_cable_abnormal
            );

            #ifdef LOGGER_DEBUG
                Serial.println();
                Serial.println(dataToBeSaved);
                Serial.printf("\nSize: %ul", sizeof(dataToBeSaved));
            #endif

            int writeOperationStatus = fprintf( this->logFile, dataToBeSaved  );
            fclose(this->logFile);
            this->logFile = NULL;
            if (  writeOperationStatus < 0 ) {
                Serial.println("Error on writing message");
                
                //  Increase the update attempt count
                this->updateAttemptCount++;
                //  If attempt tryout, stop logging service
                if ( this->updateAttemptCount >= this->updateAttemptMax ) {
                    Serial.println("[FAIL]      Update file attempt tryout reached. Stoping the recording service.");
                    this->stopLog();
                    return;
                };
            } else { this->updateAttemptCount = 0; }

            return;
        }
};
LOGGER SDlogger;