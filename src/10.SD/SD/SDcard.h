#include <SD.h>

#define SCK GPIO_NUM_14
#define MISO GPIO_NUM_2
#define MOSI GPIO_NUM_15
#define SS GPIO_NUM_13

class SDCard {
    private:
        int16_t mountFailCount = 0;
        int16_t mountFailMax = 5;

    public:

        int mountSD(){

            //  Reset the fail count
            this->mountFailCount  = 0;

            int wasSerialOpen = 0;
            if ( Serial ) {
                wasSerialOpen = 1;
                Serial.end();
            }

            //  If the card is mounted, simply return true. If isn't, re-mount SD
            if ( SD.exists("/") ) {
                if ( wasSerialOpen ) Serial.begin(115200);
                return 1;
            } else {
                if ( wasSerialOpen ) Serial.begin(115200);

                for ( int i = 0; i < this->mountFailMax; i++ ) {

                    //  Mount the SD card
                    if ( SD.begin( SS, SPI ) ) return 1;

                    vTaskDelay( 750 / portTICK_PERIOD_MS );
                }

                return 0;
            }
        }

        int unmountSD(){
            SD.end();
            return 1;
        }

        void setup(){

            pinMode( SS, OUTPUT );
            
            SPI.begin(SCK, MISO, MOSI, SS);
            
            //  If the card is not mounted
            if ( this->mountSD() ) Serial.println("[OK]     SD mount compleated");
            else Serial.println("[ERROR]       Error on mounting SD card. Check if SD card is properlly placed and try again");
        }

};

SDCard sd_card;