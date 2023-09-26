#include <Arduino.h>

class AntiCrashClass {
    private:
        File crashCheck;
        char crashFile[15] = "/antiCrash.txt";

        //  When the check file is non-existing or is previouslly set to 0
        void createCrashCheckFile () {

            SPIFFS.begin();
            this->crashCheck = SPIFFS.open(this->crashFile, FILE_WRITE);
            this->crashCheck.write('1');
            this->crashCheck.close();
            SPIFFS.end();
            utilities.ESPReset();
        }

        //  When the check file is ok and needs to be set to 0
        void updateCrashCheckFile() {
            SPIFFS.begin();
            this->crashCheck = SPIFFS.open(this->crashFile, FILE_WRITE);
            this->crashCheck.write('1');
            this->crashCheck.close();
            SPIFFS.end();
        }

    public:
        void checkLastCrash(){

            SPIFFS.begin();
            
            Serial.print("Checking for hardware reboot integrity");

            this->crashCheck = SPIFFS.open(this->crashFile, FILE_READ);
            // if file doesn't exists, create one and allow it to continue the program on next MCU reset
            if ( !this->crashCheck ) {
                Serial.println("        [FAIL]");
                Serial.println("Fixing hardware reboot integrity and rebooting");
                this->createCrashCheckFile();
            }
            
            int isAllowedSize = 1;
            char isAllowed[this->crashCheck.size() + 1];
            int loop_read = 0;

            while ( this->crashCheck.available() && loop_read < isAllowedSize ) {
                isAllowed[loop_read] = this->crashCheck.read();
                loop_read++;
            }
            isAllowed[loop_read] = '\0';

            this->crashCheck.close();
            SPIFFS.end();

            if (strcmp(isAllowed, "1") == 0) {
                Serial.println("        [OK]");
                Serial.println("Hardware still need to reboot though");

                this->crashCheck.close();
                this->createCrashCheckFile();
                return;
            }
            else {
                Serial.println("        [OK]");

                //  If it was no longer needing for hardware reboot, set it to pending and continue the program
                // In that way, the next time the hardware boot, it will hardware reboot once to clean the memory
                this->updateCrashCheckFile();
            }
        }

};
AntiCrashClass AntiCrash;