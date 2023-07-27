class WHISTLEBLOWING {
    public:
        int isWissleblowingActive = 0;

    private:
        char stateFile[18] = "/wasStreaming.txt";

        int flagStart() {
            SPIFFS.begin();
            File recordingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
            recordingFile.write('1');
            recordingFile.close();
            SPIFFS.end();

            return 1;
        }

        int flagEnd() {
            SPIFFS.begin();
            File recordingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
            recordingFile.write('0');
            recordingFile.close();
            SPIFFS.end();

            return 1;
        }

    public:

        #ifdef SPIFFS_H
        /* Auto start streaming service
        This method aims to restart the streaming service if it was suddenly interrupted  on the
        previous runtime
        0 - Failed
        1 - Success*/
        int autoStartWhistleblowing(){

            Serial.print("Check if it was Streaming data on the last runtime");

            SPIFFS.begin();
            File streamingFile = SPIFFS.open(this->stateFile, FILE_READ);

            if ( !streamingFile ) {
                Serial.print("      [FAIL]");
                Serial.println("Last streaming state file does not exist. Creating one and continuing the program");

                streamingFile = SPIFFS.open(this->stateFile, FILE_WRITE);
                streamingFile.write('0');
                streamingFile.close();
                SPIFFS.end();

                return 0;
            }

            int previousStreamingStateLen = 1;
            char previousStreamingState[ previousStreamingStateLen + 1 ];
            int loop_read = 0;

            while ( streamingFile.available() && loop_read < previousStreamingStateLen ) {
                previousStreamingState[ loop_read ] = streamingFile.read();
                loop_read++;
            }
            previousStreamingState[ loop_read ] = '\0';

            streamingFile.close();

            if ( strcmp( previousStreamingState, "1" ) != 0 ) {
                Serial.println("      [OK]");
                Serial.println("It was not streaming on the previous runtime. Continuing the program");
                SPIFFS.end();
                return 0;
            } else {
                Serial.println("      [OK]");
                Serial.println("Resuming the streaming service started on the previous runtime");
                this->startWhistleblowing();
                return 1;
            }
        }

        #endif

        /*  1 - Success
            2 - Failed to open log file
            0 - Failed, service is already active
        */
        int startWhistleblowing(){

            if ( !this->isWissleblowingActive ) {
                if ( this->flagStart() ) {

                    //  Flags that whissleblowing service is now active
                    this->isWissleblowingActive = 1;

                    //  Turn the ESPServer off if it is connected
                    ESPServer.turnESPServerOff();

                    Serial.print("Resuming the message system");

                    // Resume the high frequency task
                    toggleHighFreq( true );

                    //  Resume the medium frequency task
                    toggleMediumFreq( true );

                    //  Resume the MQTT delivery task
                    toggleMQTTFreq( true );

                    //  Resume the Heartbeat detection task
                    vTaskResume(xClientsHeartbeat);

                    return 1;
                } else return 2;
            } else return 0;
            
        }

        /*  1 - Success
            2 - Failed to open log file
            0 - Failed, service is already inactive
        */
        int stopWhistleblowing(){

            if ( this->isWissleblowingActive ) {
                if ( this->flagEnd() ) {

                    // Suspend the high frequency task
                    toggleHighFreq( false );

                    //  Suspend the medium frequency task
                    toggleMediumFreq( false );

                    //  Suspend the MQTT delivery task
                    toggleMQTTFreq( false );

                    //  Suspend the Heartbeat detection task
                    vTaskSuspend(xClientsHeartbeat);

                    //  Flags that whissleblowing service is now inactive
                    this->isWissleblowingActive = 0;

                    return 1;
                } else return 2;
            } else return 0;
        }
};
WHISTLEBLOWING wb;