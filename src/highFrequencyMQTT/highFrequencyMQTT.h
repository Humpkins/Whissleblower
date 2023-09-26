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
            if ( status ) { sim_7000g.last_message = xTaskGetTickCount(); }

            if ( !status )  return false;
            else return true;

        }

        int sendMHighFrequencyDataOverMQTTHEX(){
            //  float   float       float       int16_t       int16_t   int16_t
            //  xxx     xxx         xxx         xxx         xxx     xxx
            //  Y       P           R           Acx         Acy     Acz

            //  int16_t     int16_t     int16_t     int16_t     int16_t
            //  xxx         xxx         xxx         xxx         xxx
            //  A1          V1          soc1        Soh1        BM1T

            //  int16_t     int16_t     int16_t     int16_t     int16_t
            //  xxx         xxx         xxx         xxx         xxx
            //  soc1        Soc2        Soh1        Soh2        BM2T

            //  int16_t     int16_t     int16_t     int16_t     int8_t
            //  xxxx        xxx         xxx         xxx         x
            //  motorspd    motortrq    MtrTemp     CtrlTemp    recording

            char HighFrequencyDataBuffer[150];

            // int8_t totalSize = 1*sizeof(int8_t) + 3*sizeof(float) + 17*sizeof(int16_t);

            // unsigned char* sendData = (unsigned char*)malloc(totalSize + 2);

            // //(float) Yaw
            // float * yaw = (float * )sendData;
            // * yaw = MPU_DATA.MPUData.ypr[0];

            // //(float) Pitch
            // float * pitch = (float * )( sendData + 1 * sizeof(float));
            // * pitch = MPU_DATA.MPUData.ypr[1];

            // //(float) Roll
            // float * roll = (float * )( sendData + 2 * sizeof(float));
            // * roll = MPU_DATA.MPUData.ypr[2];

            // //(int16_t) AccX
            // int16_t * accX = (int16_t * )( sendData + 3 * sizeof(float));
            // * accX = MPU_DATA.MPUData.aaWorld.x;

            // //(int16_t) AccY
            // int16_t * accY = (int16_t * )( sendData + 3 * sizeof(float) + 1 * sizeof(int16_t));
            // * accY = MPU_DATA.MPUData.aaWorld.y;

            // //(int16_t) AccZ
            // int16_t * accZ = (int16_t * )( sendData + 3 * sizeof(float) + 2 * sizeof(int16_t));
            // * accZ = MPU_DATA.MPUData.aaWorld.z;

            // //(int) Bat_current_1
            // int * batt_current_1 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t));
            // * batt_current_1 = TJA_DATA.batteries[0].current;

            // //(int) Bat_voltage_1
            // int * batt_voltage_1 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 1 * sizeof(int));
            // * batt_voltage_1 = TJA_DATA.batteries[0].voltage;

            // //(int) Bat_SoC_1
            // int * batt_soc_1 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 2 * sizeof(int));
            // * batt_soc_1 = TJA_DATA.batteries[0].SoC;

            // //(int) Bat_SoH_1
            // int * batt_soh_1 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 3 * sizeof(int));
            // * batt_soh_1 = TJA_DATA.batteries[0].SoH;

            // //(int) Bat_temp_1
            // int * batt_temp_1 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 4 * sizeof(int));
            // * batt_temp_1 = TJA_DATA.batteries[0].temperature;

            // //(int) Bat_current_2
            // int * batt_current_2 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 5 * sizeof(int));
            // * batt_current_2 = TJA_DATA.batteries[1].current;

            // //(int) Bat_voltage_2
            // int * batt_voltage_2 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 6 * sizeof(int));
            // * batt_voltage_2 = TJA_DATA.batteries[1].voltage;

            // //(int) Bat_SoC_2
            // int * batt_soc_2 = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 7 * sizeof(int));
            // * batt_soc_2 = TJA_DATA.batteries[1].SoC;

            // //(int) Bat_SoH_2
            // float * batt_soh_2 = (float * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 8 * sizeof(int));
            // * batt_soh_2 = TJA_DATA.batteries[1].SoH;

            // //(int) Bat_temp_2
            // float * batt_temp_2 = (float * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 9 * sizeof(int));
            // * batt_temp_2 = TJA_DATA.batteries[1].temperature;

            // //(int) Motor_speed_RPM
            // int * Motor_speed_RPM = (int * )( sendData + 3 * sizeof(float) + 3 * sizeof(int16_t) + 10 * sizeof(int));
            // * Motor_speed_RPM = TJA_DATA.CurrentPowertrainData.motorSpeedRPM;

            // high_freq_root["Motor_Speed_RPM"] =    TJA_DATA.CurrentPowertrainData.motorSpeedRPM;
            // high_freq_root["Motor_Torque_Nm"] =    TJA_DATA.CurrentPowertrainData.motorTorque;
            // high_freq_root["Motor_Temperature_C"] =    TJA_DATA.CurrentPowertrainData.motorTemperature;
            // high_freq_root["Controller_Temperature_C"] =    TJA_DATA.CurrentPowertrainData.controllerTemperature;

            // high_freq_root["recording"] = SDlogger.recording;

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
            else {
                sim_7000g.last_message = xTaskGetTickCount();
                return true;
            }
        }

};

highFrequencyMQTT highFrequency;