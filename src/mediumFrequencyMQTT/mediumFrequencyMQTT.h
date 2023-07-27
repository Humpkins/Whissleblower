#include <Arduino.h>
#include <ArduinoJson.h>

StaticJsonDocument<384> medium_freq_root;

class mediumFrequencyMQTT {
    private:
        float BatteryVoltageDivider = ( (100000 + 100000) / 100000 );

    public:

        // Send Medium frequency data to the MQTT Broker
        bool sendMediumFrequencyDataOverMQTT(){

            // Pack GPS data in JSON
            medium_freq_root["Latitude"] =                  sim_7000g.CurrentGPSData.latitude;
            medium_freq_root["Longitude"] =                 sim_7000g.CurrentGPSData.longitude;
            medium_freq_root["GPS_Speed"] =                 sim_7000g.CurrentGPSData.speed;
            medium_freq_root["Altitude"] =                  sim_7000g.CurrentGPSData.altitude;
            medium_freq_root["Orientation"] =               sim_7000g.CurrentGPSData.orientation;
            medium_freq_root["SatelitesInViewGNSS"] =       sim_7000g.CurrentGPSData.vSatGNSS;
            medium_freq_root["SatelitesInUseGPS"] =         sim_7000g.CurrentGPSData.uSatGPS;
            medium_freq_root["SatelitesInUseGLONASS"] =     sim_7000g.CurrentGPSData.uSatGLONASS;

            // Pack GPRS data in JSON
            medium_freq_root["GPRS_SingalQuality"] =        sim_7000g.CurrentGPRSData.signalQlty;
            medium_freq_root["GPRS_Operational_Mode"] =     sim_7000g.CurrentGPRSData.operationalMode;
            medium_freq_root["cellID"] =                    sim_7000g.CurrentGPRSData.cellID;
            medium_freq_root["MCC"] =                       sim_7000g.CurrentGPRSData.MCC;
            medium_freq_root["MNC"] =                       sim_7000g.CurrentGPRSData.MNC;
            medium_freq_root["LAC"] =                       sim_7000g.CurrentGPRSData.LAC;
            medium_freq_root["Firmware_Ver"] =              g_states.Ver;
            medium_freq_root["ICCID"] =                     sim_7000g.CurrentGPRSData.ICCID;
            int16_t readVoltage = analogRead(ESP_BAT_PIN); 
            float batVoltage = ( readVoltage * 3.3 * BatteryVoltageDivider) / 4095;
            medium_freq_root["bat_voltage"] =               ((readVoltage == 4095 || readVoltage == 0))?99:batVoltage;

            // Serialize JSON Object to array of string
            char MediumFrequencyDataBuffer[measureJson(medium_freq_root) + 1];
            serializeJson(medium_freq_root, MediumFrequencyDataBuffer, measureJson(medium_freq_root) + 1);

            // Send data to Medium Frequency topic
            char topic[sizeof(g_states.MQTTProject)+ sizeof(g_states.MQTTclientID)+ sizeof(g_states.MQTTMediumFrequencyTopic) + 3];
            sprintf(topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTMediumFrequencyTopic);

            int status = mqtt.publish(topic, MediumFrequencyDataBuffer);

            if ( !status ) return false;
            else return true;
        }

        // Send Medium frequency data to the MQTT Broker
        bool sendMediumFrequencyDataOverMQTTHEX(){
            //  xxxxxxxxxxxxxx      xxxxxxxxxxxxxx      xxx         xxxxxxxxxxxxxx      xxx
            //  Lat                 Lng                 GPSSpd      Alt                 Orient

            //  xxx                 xxx             xxx
            //  SatInViewGNSS       SatInUseGPS     SatInUseGLONASS

            //  xx      xxxxxxxxxxxx      xxxxxxxxx     xxx     x       xxxxx
            //  Qlty    OPMode            CellID        MCC     MNC     LAC

            //  xxxxxx      xxxxxxxxxxxxxxxxxxxx
            //  Firm        ICCID

            char MediumFrequencyDataBuffer[150];

            sprintf(
                MediumFrequencyDataBuffer,
                "%+014.9f%+014.9f%005.2f%+014.9f%005.2f%03d%03d%03d%02d%-*s%09d%03d%01d%05d%-*s%020d",

                // Pack GPS data in JSON
                sim_7000g.CurrentGPSData.latitude,
                sim_7000g.CurrentGPSData.longitude,
                sim_7000g.CurrentGPSData.speed,
                sim_7000g.CurrentGPSData.altitude,
                sim_7000g.CurrentGPSData.orientation,
                sim_7000g.CurrentGPSData.vSatGNSS,
                sim_7000g.CurrentGPSData.uSatGPS,
                sim_7000g.CurrentGPSData.uSatGLONASS,

                // Pack GPRS data in JSON
                sim_7000g.CurrentGPRSData.signalQlty,
                12,
                sim_7000g.CurrentGPRSData.operationalMode,
                sim_7000g.CurrentGPRSData.cellID,
                sim_7000g.CurrentGPRSData.MCC,
                sim_7000g.CurrentGPRSData.MNC,
                sim_7000g.CurrentGPRSData.LAC,
                7,
                g_states.Ver,
                sim_7000g.CurrentGPRSData.ICCID
            );  

            // Send data to Medium Frequency topic
            char topic[sizeof(g_states.MQTTProject)+ sizeof(g_states.MQTTclientID)+ sizeof(g_states.MQTTMediumFrequencyTopic) + 3];
            sprintf(topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTMediumFrequencyTopic);

            int status = mqtt.publish(topic, MediumFrequencyDataBuffer, false);

            if ( !status ) return false;
            else return true;
        }
};

mediumFrequencyMQTT mediumFrequency;