class overcurrent {

    public:
        FILE *watcherFile;
        int16_t isSetedUp = 0;
        char stateFile[18] = "/overcurrent.txt";

        void setup(){
            
            //  If the card is not mounted
            if ( sd_card.mountSD() ) Serial.println("[OK]     SD mount compleated");
            else Serial.println("[ERROR]       Error on mounting SD card. Check if SD card is properlly placed and try again");
        }

        void current() {

            // Check for overcurrent for each battery. If encounters overcurrented battery, logs it to a file
            for ( int i = 0; i < sizeof(TJA_DATA.batteries)/sizeof(TJA_DATA.batteries[0]); i++ ) {
                if ( TJA_DATA.batteries[i].current > g_states.MAX_CURRENT ) {

                    //  Close non-related overcurrent files
                    this->watcherFile = fopen(this->stateFile, FILE_WRITE);

                    char * buffer;
                    sprintf(
                        buffer,
                        "%d, %.8f, %.8f, %s, %d\n",
                        TJA_DATA.batteries[i].current,
                        sim_7000g.CurrentGPSData.latitude,
                        sim_7000g.CurrentGPSData.longitude,
                        sim_7000g.CurrentGPSData.datetime,
                        i
                    );

                    fclose(this->watcherFile);
                }
            }
            return;
        }
};

overcurrent WatcherCurrent;