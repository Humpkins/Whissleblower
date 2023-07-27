//#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_MODEM_SIM7000
// #define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

#ifndef RECONNECT_ATTEMPT_LIMIT
    #define RECONNECT_ATTEMPT_LIMIT 2
#endif

// // Caso eu queria debugar os comandos AT mandados para o SIM800L
// #define DUMP_AT_COMMANDS
// #define SerialMon Serial


//  If is needed to use LTE instead of GPRS
#define LTE

#include <TinyGsmClient.h>
#include <time.h>

#ifndef CONFIG_DATA
    #include "../config.h"
#endif

// TTGO T-Call pins
#define MODEM_RST            GPIO_NUM_5
#define MODEM_PWKEY          GPIO_NUM_4
#define MODEM_POWER_ON       GPIO_NUM_25
#define MODEM_TX             GPIO_NUM_27
#define MODEM_RX             GPIO_NUM_26
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 200000
// Set serial for debug console (to the Serial Monitor, default speed 115200)

#define SerialAT Serial1

#ifdef DUMP_AT_COMMANDS
    #include <StreamDebugger.h>
    StreamDebugger debugger(SerialAT, SerialMon);
    TinyGsm        modem(debugger);
    #else
    TinyGsm        modem(SerialAT);
#endif

TinyGsmClient GSMclient( modem, 0 );

class SIM7000G {

    public:

        void turnGPSOn(void){

            SerialAT.print("AT+SGPIO=0,4,1,1\r\n");
            vTaskDelay( 500 / portTICK_PERIOD_MS );
            SerialAT.print("AT+CGNSPWR=1\r\n");
            vTaskDelay( 500 / portTICK_PERIOD_MS );
            SerialAT.print("AT+CGNSHOT\r\n");
            vTaskDelay( 500 / portTICK_PERIOD_MS );
        }

        void turnGPSOff(void){

            SerialAT.print("AT+SGPIO=0,4,1,0\r\n");
            vTaskDelay( 500 / portTICK_PERIOD_MS );
            SerialAT.print("AT+CGNSPWR=0\r\n");
            vTaskDelay( 500 / portTICK_PERIOD_MS );

        }

    public:
        // Creates a struct to store GPS medium frequency info
        struct gpsInfo {
            float latitude = 0.0;
            float longitude = 0.0;
            float speed = 0.0;
            float altitude = 0.0;
            float orientation = 0.0;
            int vSatGNSS = 0;
            int uSatGPS = 0;
            int uSatGLONASS = 0;
            char datetime[64] = "00_00_0000 00-00-00";
            int timezone = 0;
        };
        // Instantiate the GPS structure
        struct gpsInfo CurrentGPSData;

        // Creates a struct to store GPRS data
        struct gprsInfo {
            int signalQlty = 0;
            char operationalMode[11] = "";
            
            //  Used to locate GSM tower's location
            long cellID = 0;
            int MCC = 0;
            int MNC = 0;
            int LAC = 0;
            char ICCID[21] = "";
        };
        // Instantiate the GPRS structure
        struct gprsInfo CurrentGPRSData;

        void setup(){

            pinMode( MODEM_PWKEY, OUTPUT );
            digitalWrite( MODEM_PWKEY, HIGH );

            SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX, false);

            vTaskDelay( 1500 / portTICK_PERIOD_MS );

            if ( !digitalRead(MODEM_PWKEY) ) modem.init();
            else modem.restart();

            Serial.print("\nConnecting to APN ");
            Serial.print(g_states.APN_GPRS);
            SerialAT.print("AT+SGPIO=0,4,1,1\r\n");

            #ifdef LTE
                modem.setNetworkMode(38);
                modem.setPreferredMode(1);
            #else
                modem.setNetworkMode(13);
                modem.setPreferredMode(3);
            #endif

            if ( modem.gprsConnect(g_states.APN_GPRS, g_states.APNUser, g_states.APNPassword) ) {
                Serial.println("       [OK]");
            }
            else {
                Serial.println("       [RECONNECTION FAIL]");
                Serial.println("[ERROR]    Handdle APN connection RECONNECTION fail");
                utilities.ESPReset();
            }
            
            if ( !modem.isGprsConnected() ){
                Serial.print("Connecting to GPRS: ");
                if (modem.isGprsConnected()){
                    Serial.println("       [OK]");

                } else {
                    Serial.println("       [RECONNECTION FAIL]");
                    Serial.println("Handlle GPRS connection RECONNECTION fail");
                    utilities.ESPReset();
                }
            }

            Serial.print("Seting up the RTC");
            if ( this->setupDateTime() ) {
                Serial.println("        [OK]");
                Serial.printf( "Current datetime: %s\n", this->CurrentGPSData.datetime );
            } else {
                Serial.println("        [ERROR]");
                Serial.println("Handle RTC setup error");
                utilities.ESPReset(  );
            }

        }

        bool checkConnection(){

            int tryout = 5;
            for ( int i = 0; i < tryout; i++ ) {

                SerialAT.print("AT+CGATT?\r\n");
                vTaskDelay( 500 / portTICK_PERIOD_MS );

                // Read the AT command's response
                char response[10];  // Buffer for the AT command's response
                char midBuff = '\n';
                int position = 0;   // position in array inside the loop 
                while ( SerialAT.available() ){
                    midBuff = SerialAT.read();

                    //  read until the linebreak
                    if ( midBuff == '\n' ) break;

                    response[position] = midBuff;
                    position++;
                    if ( position > 9 ) break;
                }
                response[position] = '\0';
                //  Clear Serial buffer
                Serial.flush();

                //  If the response is like expected, return true
                if ( strstr( response, "+CGATT: 1" ) == NULL ) return true;
            }
            //  If tryout limit is reached, return false
            return false;
        }

        // Function to reconect to GPRS if it is disconnected
        void maintainGPRSconnection(){
            // If there is no GPRS connection, then try to connect

            for ( int attempt = 0; attempt < RECONNECT_ATTEMPT_LIMIT; attempt++ ){

                Serial.print(attempt);
                Serial.print("    Reconnecting to GPRS");

                //  Restart the modem before reconect on the last attempt
                if ( attempt == RECONNECT_ATTEMPT_LIMIT - 1 ) {
                    modem.restart();
                    vTaskDelay( 1000 / portTICK_PERIOD_MS );
                }
                
                if ( modem.gprsConnect( g_states.APN_GPRS, g_states.APNUser, g_states.APNPassword ) ){
                    Serial.println("       [OK]");
                    this->turnGPSOn();
                    return;
                } else Serial.println("       [FAIL]");
            }

            Serial.println("[ERROR]    Handdle APN connection fail");
            this->restartModem();
            while(1);
        }

        bool updateGPSData(){

            char response[105]; //  <- If Stack smash ocours, increase this value
            int position = 0;

            bool retorna_true = false;

            int tryOut = 5;
            for ( int tries = 0; tries < tryOut; tries++ ) {

                SerialAT.print("AT+CGNSINF\r\n");
                vTaskDelay( 500 / portTICK_PERIOD_MS );

                position = 0;
                while ( SerialAT.available() && position < 100 ){
                    response[position] = SerialAT.read();
                    position++;
                }
                response[position] = '\0';
                //  Clear Serial buffer
                Serial.flush();

                if ( strstr( response, "+CGNSINF: 1,1" ) == NULL ) continue;
                else {
                    sscanf(
                        response,
                        "\n+CGNSINF: 1,1,%*f,%f,%f,%f,%f,%f,%*[0-2],,%*[0-99.9],%*[0-99.9],%*[0-99.9],,%d,%d,%d",
                        &this->CurrentGPSData.latitude,
                        &this->CurrentGPSData.longitude,
                        &this->CurrentGPSData.altitude,
                        &this->CurrentGPSData.speed,
                        &this->CurrentGPSData.orientation,
                        &this->CurrentGPSData.vSatGNSS,
                        &this->CurrentGPSData.uSatGPS,
                        &this->CurrentGPSData.uSatGLONASS
                    );

                    // If read, return true
                    return true;
                    retorna_true = true;
                    break;
                }

            }

            //  If could not lock GPS data in <tryOut> times, return false
            return retorna_true;

        }

        void ForceConnectToAPN(){

            SerialAT.printf("AT+CSTT=\"%s\",\"%s\",\"%s\"\r\n", g_states.APN_GPRS, g_states.APNUser, g_states.APNPassword);
            vTaskDelay( 100 / portTICK_PERIOD_MS );

            SerialAT.print("AT+CIICR\r\n");
            vTaskDelay( 700 / portTICK_PERIOD_MS );

            SerialAT.print("AT+COPS=0");
            vTaskDelay( 700 / portTICK_PERIOD_MS );

            Serial.printf("COPS: %s\nSignalQlty: %i\nICCID: %s\n", modem.getOperator(), modem.getSignalQuality(), modem.getSimCCID());

        }

        // Warning, chaotic function ahead!
        // Update de GPRS data
        bool update_GPRS_data(){

            // Update the gprs signal quality
           this-> CurrentGPRSData.signalQlty = modem.getSignalQuality();
           strcpy( this-> CurrentGPRSData.ICCID, modem.getSimCCID().c_str() );

            // Now the chaotic part: Get the Cell tower info.
            // This is messy because TinyGSM lib still doesn't have specific function for this job, so we have to parse our way out of it
            int tryOut = 5;
            for ( int tries = 0; tries < tryOut; tries++ ){

                SerialAT.print("AT+CPSI?\r\n");
                vTaskDelay( 250 / portTICK_PERIOD_MS );

                // Read the AT command's response
                char response[110];  // Buffer for the AT command's response
                int position = 0;   // position in array inside the loop 
                while ( SerialAT.available() && position < 110 ){
                    response[position] = SerialAT.read();
                    position++;
                }
                response[position] = '\0';
                //  Clear Serial buffer
                Serial.flush();

                if ( strstr( response, "+CPSI:" ) == NULL ) continue;
                else {
                    sscanf(
                        response,
                        "\n+CPSI: %[^,],%*[^,],%d-%d,%x,%ld",
                        this->CurrentGPRSData.operationalMode,
                        &this->CurrentGPRSData.MCC,
                        &this->CurrentGPRSData.MNC,
                        &this->CurrentGPRSData.LAC,
                        &this->CurrentGPRSData.cellID
                    );

                    return true;
                }

            }

            return false;
        }

        //  Sets up the microcontroller's RTC
        bool setupDateTime(){
            // modem.getGSMDateTime(DATE_FULL).toCharArray( this->CurrentGPSData.datetime, modem.getGSMDateTime(DATE_FULL).length() + 1 );

            struct tm Timestamp;
            int hh, mm, ss, yy, mon, day = 0;

            int tryout = 5;
            for ( int tries = 0; tries < tryout; tries++ ) {

                SerialAT.write("AT+CCLK?\r\n");
                vTaskDelay( 200 / portTICK_PERIOD_MS );

                char response[64];
                int position = 0;
                while ( SerialAT.available() ) {
                    response[position] = SerialAT.read();
                    position++;
                }
                response[position] = '\0';

                if ( strstr( response, "+CCLK: " ) == NULL ) continue;
                else {
                    struct tm Timestamp;
                    sscanf( response, "\n+CCLK: \"%d/%d/%d,%d:%d:%d%d\"\nOK\n", &yy, &mon, &day, &hh, &mm, &ss, &this->CurrentGPSData.timezone );

                    Timestamp.tm_hour = hh;
                    Timestamp.tm_min = mm;
                    Timestamp.tm_sec = ss;
                    Timestamp.tm_year = 2000 + yy - 1900;

                    Timestamp.tm_mon = mon - 1;
                    Timestamp.tm_mday = day;
                    Timestamp.tm_isdst = -1;

                    time_t epoch = mktime( &Timestamp );

                    struct timeval tv;
                    tv.tv_sec = epoch + 1;
                    tv.tv_usec = 0;
                    settimeofday( &tv, NULL );

                    if ( epoch > 0 ) {
                        this->updateDateTime();
                        return true;
                    }
                }
            }

            return false;
        }

        void updateDateTime(){

            time_t tt = time(NULL);
            time_t tt_with_offset = tt + ( 3600 * (this->CurrentGPSData.timezone/2) );

            struct tm Timestamp;

            Timestamp = *gmtime(&tt_with_offset);

            strftime(this->CurrentGPSData.datetime, 64, "%d/%m/%Y %H:%M:%S", &Timestamp);
        }

        void restartModem(){
            Serial.println("Restarting modem");
            if ( xSemaphoreTake( xModem, portMAX_DELAY ) == pdTRUE ) {
                modem.restart();
                vTaskDelay( 2000 / portTICK_PERIOD_MS );
                xSemaphoreGive( xModem );
            }
        }
};

SIM7000G sim_7000g;