#include <Arduino.h>
#include <ArduinoJson.h>

StaticJsonDocument<384> high_freq_root;

class highFrequencyMQTT {

    public:

        // Send Medium frequency data to the MQTT Broker
        int sendMHighFrequencyDataOverMQTT(){
            //  IMU data
            high_freq_root["Yaw"] = MPU_DATA.MPUData.ypr[0];
            high_freq_root["Pitch"] = MPU_DATA.MPUData.ypr[1];
            high_freq_root["Roll"] = MPU_DATA.MPUData.ypr[2];
            high_freq_root["Acell_x"] = MPU_DATA.MPUData.aaWorld.x;
            high_freq_root["Acell_y"] = MPU_DATA.MPUData.aaWorld.y;
            high_freq_root["Acell_z"] = MPU_DATA.MPUData.aaWorld.z;

            // Pack Battery data in JSON
            high_freq_root["BMS1_Current"] = TJA_DATA.batteries[0].current;
            high_freq_root["BMS1_Voltage"] = TJA_DATA.batteries[0].voltage;
            high_freq_root["BMS1_SoC"] = TJA_DATA.batteries[0].SoC;
            high_freq_root["BMS1_SoH"] = TJA_DATA.batteries[0].SoH;
            high_freq_root["BMS1_Temperature"] = TJA_DATA.batteries[0].temperature;

            high_freq_root["BMS2_Current"] = TJA_DATA.batteries[1].current;
            high_freq_root["BMS2_Voltage"] = TJA_DATA.batteries[1].voltage;
            high_freq_root["BMS2_SoC"] = TJA_DATA.batteries[1].SoC;
            high_freq_root["BMS2_SoH"] = TJA_DATA.batteries[1].SoH;
            high_freq_root["BMS2_Temperature"] = TJA_DATA.batteries[1].temperature;

            // Pack powertrain data in JSON
            high_freq_root["Motor_Speed_RPM"] =    TJA_DATA.CurrentPowertrainData.motorSpeedRPM;
            high_freq_root["Motor_Torque_Nm"] =    TJA_DATA.CurrentPowertrainData.motorTorque;
            high_freq_root["Motor_Temperature_C"] =    TJA_DATA.CurrentPowertrainData.motorTemperature;
            high_freq_root["Controller_Temperature_C"] =    TJA_DATA.CurrentPowertrainData.controllerTemperature;

            high_freq_root["recording"] = SDlogger.recording;
            

            //  Serialize JSON Object to array of string
            char HighFrequencyDataBuffer[measureJson(high_freq_root) + 1];
            serializeJson(high_freq_root, HighFrequencyDataBuffer, measureJson(high_freq_root) + 1);

            // Send data to Medium Frequency topic

            char topic[sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTHighFrequencyTopic) + 3];
            sprintf(topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTHighFrequencyTopic);

            int status = mqtt.publish(topic, HighFrequencyDataBuffer);

            if ( !status )  return false;
            else return true;

        }

        int sendMHighFrequencyDataOverMQTTHEX(){
            //  xxx     xxx         xxx         xxx         xxx     xxx
            //  Y       P           R           Acx         Acy     Acz

            //  xxx     xxx     xxx     xxx     xxx
            //  A1      V1      soc1    Soh1    BM1T

            //  xxx     xxx     xxx     xxx     xxx
            //  soc1    Soc2    Soh1    Soh2    BM2T

            //  xxxx        xxx         xxx         xxx         x
            //  motorspd    motortrq    MtrTemp     CtrlTemp    recording

            char HighFrequencyDataBuffer[150];
            
            sprintf(
                //-000.057085775
                HighFrequencyDataBuffer,
                "%+014.9f%+014.9f%+014.9f%+03d%+03d%+03d%03d%03d%03d%03d%03d%03d%03d%03d%03d%03d%04d%03d%03d%03d%01d",

                //  IMU data
                MPU_DATA.MPUData.ypr[0],
                MPU_DATA.MPUData.ypr[1],
                MPU_DATA.MPUData.ypr[2],
                MPU_DATA.MPUData.aaWorld.x,
                MPU_DATA.MPUData.aaWorld.y,
                MPU_DATA.MPUData.aaWorld.z,

                // Pack Battery data
                TJA_DATA.batteries[0].current,
                TJA_DATA.batteries[0].voltage,
                TJA_DATA.batteries[0].SoC,
                TJA_DATA.batteries[0].SoH,
                TJA_DATA.batteries[0].temperature,

                TJA_DATA.batteries[1].current,
                TJA_DATA.batteries[1].voltage,
                TJA_DATA.batteries[1].SoC,
                TJA_DATA.batteries[1].SoH,
                TJA_DATA.batteries[1].temperature,

                // Pack powertrain data
                TJA_DATA.CurrentPowertrainData.motorSpeedRPM,
                TJA_DATA.CurrentPowertrainData.motorTorque,
                TJA_DATA.CurrentPowertrainData.motorTemperature,
                TJA_DATA.CurrentPowertrainData.controllerTemperature,

                // Recording status
                (SDlogger.recording)?1:0
            );

            // Send data to Medium Frequency topic
            char topic[sizeof(g_states.MQTTProject) + sizeof(g_states.MQTTclientID) + sizeof(g_states.MQTTHighFrequencyTopic) + 3];
            sprintf(topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, g_states.MQTTHighFrequencyTopic);

            int status = mqtt.publish(topic, HighFrequencyDataBuffer, false);

            if ( !status )  return false;
            else return true;
        }

};

highFrequencyMQTT highFrequency;