#include <Arduino.h>

class UTILS {
    
    public:
        QueueHandle_t * TJAqueueHandler = NULL;

        bool ParseCharArray( char * desiredChar, char * response,
                                int position, char splitter, bool isLast = false ){

            // Split the response's parameters position in the char array
            int splitter_count = 0;
            for ( int i = 0; i < sizeof(response); i++){
                
                // If finds aplitter, check if is the desired position. If not, increase splitter_count
                if ( response[i] == splitter && splitter_count != position ) splitter_count++;

                // If splitter_count matches the desired position, start store the incomming char
                if ( splitter_count == position ) {
                    int ArrPosition = 0;               // position in array inside the loop
                    // Reads incomming char until finds another splitter or array end
                    for ( int j = i + 1; (!isLast)?response[j] != splitter:response[j] != '\0'; j++ ) {
                        desiredChar[ ArrPosition ] = response[j];
                        ArrPosition++;
                    }
                    desiredChar[ ArrPosition ] = '\0';
                    return true;
                }
            }

            return false;
        }

        //  Replace all occourrences of specified char
        char * replace_char(char * str, char find, char replace ){
            char * current_pos = strchr(str,find);
            while (current_pos) {
                * current_pos = replace;
                current_pos = strchr(current_pos,find);
            }
            return str;
        }

        void ESPClearFlash(){
            //  Flash erase process
            esp_err_t result;
            const esp_partition_t* firmware_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
            const esp_partition_t* spiffs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);

            // Erase all partitions except for the SPIFFS partition and firmware partition
            result = esp_partition_erase_range(firmware_partition, firmware_partition->size, spiffs_partition->address - firmware_partition->address);
        }

        //  Propperly software reset ESP
        void ESPReset() {
            #ifdef TESTING_CAN
                if ( TJAqueueHandler != NULL ) vQueueDelete( * TJAqueueHandler );
            #endif

            /* PENDING FOR IMPLEMENTATION*/
            /* Before ESP restar, close the open files and unmount the SD card */
            // if ( SDlogger.recording ) {
            //     SDlogger.stopLog();
            // }

            pinMode( GPIO_NUM_25 ,OUTPUT);
            pinMode( GPIO_NUM_32 ,OUTPUT);

            
            vTaskDelay( 1000 / portTICK_PERIOD_MS );

            this->ESPClearFlash();

            esp_restart();
            // digitalWrite( ESP_RST_PIN, LOW );

        }

        bool containsOnlyASCII(const std::string& subject) {
            for (auto c: subject) {
                if (static_cast<unsigned char>(c) > 127) return false;
            }
            return true;
        }

};

UTILS utilities;