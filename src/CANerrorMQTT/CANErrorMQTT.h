#include <Arduino.h>
#include <ArduinoJson.h>

StaticJsonDocument<768> CANErr_freq_root;

class CANErrorMQTT {
    public:
        int timeToSend = 1;
        int MQTTpacketSent[3] = {0, 0, 0};

        bool sendCANErrorDataOverMQTT (){

            CANErr_freq_root.clear();

            if ( TJA_DATA.MCU_error.hardwareFault1 ) { CANErr_freq_root["hardwareFault1"] = TJA_DATA.MCU_error.hardwareFault1;}
            if ( TJA_DATA.MCU_error.motorSensor ) { CANErr_freq_root["motorSensor"] = TJA_DATA.MCU_error.motorSensor;}
            if ( TJA_DATA.MCU_error.overVoltage ) { CANErr_freq_root["overVoltage"] = TJA_DATA.MCU_error.overVoltage;}
            if ( TJA_DATA.MCU_error.underVoltage ) { CANErr_freq_root["underVoltage"] = TJA_DATA.MCU_error.underVoltage;}
            if ( TJA_DATA.MCU_error.overTemperature ) { CANErr_freq_root["overTemperature"] = TJA_DATA.MCU_error.overTemperature;}
            if ( TJA_DATA.MCU_error.overCurrent ) { CANErr_freq_root["overCurrent"] = TJA_DATA.MCU_error.overCurrent;}
            if ( TJA_DATA.MCU_error.overLoad ) { CANErr_freq_root["overLoad"] = TJA_DATA.MCU_error.overLoad;}
            if ( TJA_DATA.MCU_error.motorLock ) { CANErr_freq_root["motorLock"] = TJA_DATA.MCU_error.motorLock;}
            if ( TJA_DATA.MCU_error.hardwareFault2 ) { CANErr_freq_root["hardwareFault2"] = TJA_DATA.MCU_error.hardwareFault2;}
            if ( TJA_DATA.MCU_error.hardwareFault3 ) { CANErr_freq_root["hardwareFault3"] = TJA_DATA.MCU_error.hardwareFault3;}
            if ( TJA_DATA.MCU_error.motorSensorNotConnected ) { CANErr_freq_root["motorSensorNotConnected"] = TJA_DATA.MCU_error.motorSensorNotConnected;}
            if ( TJA_DATA.MCU_error.hardwareFault4 ) { CANErr_freq_root["hardwareFault4"] = TJA_DATA.MCU_error.hardwareFault4;}
            if ( TJA_DATA.MCU_error.hardwareFault5 ) { CANErr_freq_root["hardwareFault5"] = TJA_DATA.MCU_error.hardwareFault5;}
            if ( TJA_DATA.MCU_error.motorTempSensShort ) { CANErr_freq_root["motorTempSensShort"] = TJA_DATA.MCU_error.motorTempSensShort;}
            if ( TJA_DATA.MCU_error.motorTempSensOpen ) { CANErr_freq_root["motorTempSensOpen"] = TJA_DATA.MCU_error.motorTempSensOpen;}

            // Serialize JSON Object to array of string
            char MCU_CAN_ERR[measureJson(CANErr_freq_root) + 1];
            serializeJson(CANErr_freq_root, MCU_CAN_ERR, measureJson(CANErr_freq_root) + 1);

            // Send data to Medium Frequency topic
            char MCUtopic[sizeof(g_states.MQTTProject)+ sizeof(g_states.MQTTclientID)+ sizeof(g_states.MQTTCANErrFrequencyTopic) + 3];
            sprintf(MCUtopic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, "MCU_ERR");
            int MCU_status = mqtt.publish(MCUtopic, MCU_CAN_ERR);

            CANErr_freq_root.clear();

            if ( TJA_DATA.BMS_Error[0].W_cell_chg ) {CANErr_freq_root["BMS1_W_cell_chg"] = TJA_DATA.BMS_Error[0].W_cell_chg;}
            if ( TJA_DATA.BMS_Error[0].E_cell_chg ) {CANErr_freq_root["BMS1_E_cell_chg"] = TJA_DATA.BMS_Error[0].E_cell_chg;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_overheat ) {CANErr_freq_root["BMS1_W_pkg_overheat"] = TJA_DATA.BMS_Error[0].W_pkg_overheat;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_chg_overheat ) {CANErr_freq_root["BMS1_E_pkg_chg_overheat"] = TJA_DATA.BMS_Error[0].E_pkg_chg_overheat;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_chg_undertemp ) {CANErr_freq_root["BMS1_W_pkg_chg_undertemp"] = TJA_DATA.BMS_Error[0].W_pkg_chg_undertemp;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_chg_undertemp ) {CANErr_freq_root["BMS1_E_pkg_chg_undertemp"] = TJA_DATA.BMS_Error[0].E_pkg_chg_undertemp;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_chg_overcurrent ) {CANErr_freq_root["BMS1_W_pkg_chg_overcurrent"] = TJA_DATA.BMS_Error[0].W_pkg_chg_overcurrent;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_chg_overcurrent ) {CANErr_freq_root["BMS1_E_pkg_chg_overcurrent"] = TJA_DATA.BMS_Error[0].E_pkg_chg_overcurrent;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_overvoltage ) {CANErr_freq_root["BMS1_W_pkg_overvoltage"] = TJA_DATA.BMS_Error[0].W_pkg_overvoltage;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_overvoltage ) {CANErr_freq_root["BMS1_E_pkg_overvoltage"] = TJA_DATA.BMS_Error[0].E_pkg_overvoltage;}
            if ( TJA_DATA.BMS_Error[0].E_charger_COM ) {CANErr_freq_root["BMS1_E_charger_COM"] = TJA_DATA.BMS_Error[0].E_charger_COM;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_chg_softstart ) {CANErr_freq_root["BMS1_E_pkg_chg_softstart"] = TJA_DATA.BMS_Error[0].E_pkg_chg_softstart;}
            if ( TJA_DATA.BMS_Error[0].E_chg_relay_stuck ) {CANErr_freq_root["BMS1_E_chg_relay_stuck"] = TJA_DATA.BMS_Error[0].E_chg_relay_stuck;}
            if ( TJA_DATA.BMS_Error[0].W_cell_dchg_undervoltage ) {CANErr_freq_root["BMS1_W_cell_dchg_undervoltage"] = TJA_DATA.BMS_Error[0].W_cell_dchg_undervoltage;}
            if ( TJA_DATA.BMS_Error[0].E_cell_dchg_undervoltage ) {CANErr_freq_root["BMS1_E_cell_dchg_undervoltage"] = TJA_DATA.BMS_Error[0].E_cell_dchg_undervoltage;}
            if ( TJA_DATA.BMS_Error[0].E_cell_deep_undervoltage ) {CANErr_freq_root["BMS1_E_cell_deep_undervoltage"] = TJA_DATA.BMS_Error[0].E_cell_deep_undervoltage;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_dchg_overheat ) {CANErr_freq_root["BMS1_W_pkg_dchg_overheat"] = TJA_DATA.BMS_Error[0].W_pkg_dchg_overheat;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_dchg_overheat ) {CANErr_freq_root["BMS1_E_pkg_dchg_overheat"] = TJA_DATA.BMS_Error[0].E_pkg_dchg_overheat;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_dchg_undertemp ) {CANErr_freq_root["BMS1_W_pkg_dchg_undertemp"] = TJA_DATA.BMS_Error[0].W_pkg_dchg_undertemp;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_dchg_undertemp ) {CANErr_freq_root["BMS1_E_pkg_dchg_undertemp"] = TJA_DATA.BMS_Error[0].E_pkg_dchg_undertemp;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_dchg_overcurrent ) {CANErr_freq_root["BMS1_W_pkg_dchg_overcurrent"] = TJA_DATA.BMS_Error[0].W_pkg_dchg_overcurrent;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_dchg_overcurrent ) {CANErr_freq_root["BMS1_E_pkg_dchg_overcurrent"] = TJA_DATA.BMS_Error[0].E_pkg_dchg_overcurrent;}
            if ( TJA_DATA.BMS_Error[0].W_pkg_undervoltage ) {CANErr_freq_root["BMS1_W_pkg_undervoltage"] = TJA_DATA.BMS_Error[0].W_pkg_undervoltage;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_undervoltage ) {CANErr_freq_root["BMS1_E_pkg_undervoltage"] = TJA_DATA.BMS_Error[0].E_pkg_undervoltage;}
            if ( TJA_DATA.BMS_Error[0].E_VCU_COM ) {CANErr_freq_root["BMS1_E_VCU_COM"] = TJA_DATA.BMS_Error[0].E_VCU_COM;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_dchg_softstart ) {CANErr_freq_root["BMS1_E_pkg_dchg_softstart"] = TJA_DATA.BMS_Error[0].E_pkg_dchg_softstart;}
            if ( TJA_DATA.BMS_Error[0].E_dchg_relay_stuck ) {CANErr_freq_root["BMS1_E_dchg_relay_stuck"] = TJA_DATA.BMS_Error[0].E_dchg_relay_stuck;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_dchg_short ) {CANErr_freq_root["BMS1_E_pkg_dchg_short"] = TJA_DATA.BMS_Error[0].E_pkg_dchg_short;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_temp_diff ) {CANErr_freq_root["BMS1_E_pkg_temp_diff"] = TJA_DATA.BMS_Error[0].E_pkg_temp_diff;}
            if ( TJA_DATA.BMS_Error[0].E_cell_voltage_diff ) {CANErr_freq_root["BMS1_E_cell_voltage_diff"] = TJA_DATA.BMS_Error[0].E_cell_voltage_diff;}
            if ( TJA_DATA.BMS_Error[0].E_AFE ) {CANErr_freq_root["BMS1_E_AFE"] = TJA_DATA.BMS_Error[0].E_AFE;}
            if ( TJA_DATA.BMS_Error[0].E_MOS_overtemp ) {CANErr_freq_root["BMS1_E_MOS_overtemp"] = TJA_DATA.BMS_Error[0].E_MOS_overtemp;}
            if ( TJA_DATA.BMS_Error[0].E_external_EEPROM ) {CANErr_freq_root["BMS1_E_external_EEPROM"] = TJA_DATA.BMS_Error[0].E_external_EEPROM;}
            if ( TJA_DATA.BMS_Error[0].E_RTC ) {CANErr_freq_root["BMS1_E_RTC"] = TJA_DATA.BMS_Error[0].E_RTC;}
            if ( TJA_DATA.BMS_Error[0].E_ID_conflict ) {CANErr_freq_root["BMS1_E_ID_conflict"] = TJA_DATA.BMS_Error[0].E_ID_conflict;}
            if ( TJA_DATA.BMS_Error[0].E_CAN_msg_miss ) {CANErr_freq_root["BMS1_E_CAN_msg_miss"] = TJA_DATA.BMS_Error[0].E_CAN_msg_miss;}
            if ( TJA_DATA.BMS_Error[0].E_pkg_voltage_diff ) {CANErr_freq_root["BMS1_E_pkg_voltage_diff"] = TJA_DATA.BMS_Error[0].E_pkg_voltage_diff;}
            if ( TJA_DATA.BMS_Error[0].E_chg_dchg_current_conflict ) {CANErr_freq_root["BMS1_E_chg_dchg_current_conflict"] = TJA_DATA.BMS_Error[0].E_chg_dchg_current_conflict;}
            if ( TJA_DATA.BMS_Error[0].E_cable_abnormal ) {CANErr_freq_root["BMS1_E_cable_abnormal"] = TJA_DATA.BMS_Error[0].E_cable_abnormal;}

            // Serialize JSON Object to array of string
            char BMS1_CAN_ERR[measureJson(CANErr_freq_root) + 1];
            serializeJson(CANErr_freq_root, BMS1_CAN_ERR, measureJson(CANErr_freq_root) + 1);

            // Send data to Medium Frequency topic
            char BM1topic[sizeof(g_states.MQTTProject)+ sizeof(g_states.MQTTclientID)+ sizeof(g_states.MQTTCANErrFrequencyTopic) + 3];
            sprintf(BM1topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, "BMS1_ERR");
            int BMS1_status = mqtt.publish(BM1topic, BMS1_CAN_ERR);

            CANErr_freq_root.clear();

            if ( TJA_DATA.BMS_Error[1].W_cell_chg ) {CANErr_freq_root["BMS2_W_cell_chg"] = TJA_DATA.BMS_Error[1].W_cell_chg;}
            if ( TJA_DATA.BMS_Error[1].E_cell_chg ) {CANErr_freq_root["BMS2_E_cell_chg"] = TJA_DATA.BMS_Error[1].E_cell_chg;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_overheat ) {CANErr_freq_root["BMS2_W_pkg_overheat"] = TJA_DATA.BMS_Error[1].W_pkg_overheat;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_chg_overheat ) {CANErr_freq_root["BMS2_E_pkg_chg_overheat"] = TJA_DATA.BMS_Error[1].E_pkg_chg_overheat;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_chg_undertemp ) {CANErr_freq_root["BMS2_W_pkg_chg_undertemp"] = TJA_DATA.BMS_Error[1].W_pkg_chg_undertemp;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_chg_undertemp ) {CANErr_freq_root["BMS2_E_pkg_chg_undertemp"] = TJA_DATA.BMS_Error[1].E_pkg_chg_undertemp;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_chg_overcurrent ) {CANErr_freq_root["BMS2_W_pkg_chg_overcurrent"] = TJA_DATA.BMS_Error[1].W_pkg_chg_overcurrent;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_chg_overcurrent ) {CANErr_freq_root["BMS2_E_pkg_chg_overcurrent"] = TJA_DATA.BMS_Error[1].E_pkg_chg_overcurrent;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_overvoltage ) {CANErr_freq_root["BMS2_W_pkg_overvoltage"] = TJA_DATA.BMS_Error[1].W_pkg_overvoltage;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_overvoltage ) {CANErr_freq_root["BMS2_E_pkg_overvoltage"] = TJA_DATA.BMS_Error[1].E_pkg_overvoltage;}
            if ( TJA_DATA.BMS_Error[1].E_charger_COM ) {CANErr_freq_root["BMS2_E_charger_COM"] = TJA_DATA.BMS_Error[1].E_charger_COM;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_chg_softstart ) {CANErr_freq_root["BMS2_E_pkg_chg_softstart"] = TJA_DATA.BMS_Error[1].E_pkg_chg_softstart;}
            if ( TJA_DATA.BMS_Error[1].E_chg_relay_stuck ) {CANErr_freq_root["BMS2_E_chg_relay_stuck"] = TJA_DATA.BMS_Error[1].E_chg_relay_stuck;}
            if ( TJA_DATA.BMS_Error[1].W_cell_dchg_undervoltage ) {CANErr_freq_root["BMS2_W_cell_dchg_undervoltage"] = TJA_DATA.BMS_Error[1].W_cell_dchg_undervoltage;}
            if ( TJA_DATA.BMS_Error[1].E_cell_dchg_undervoltage ) {CANErr_freq_root["BMS2_E_cell_dchg_undervoltage"] = TJA_DATA.BMS_Error[1].E_cell_dchg_undervoltage;}
            if ( TJA_DATA.BMS_Error[1].E_cell_deep_undervoltage ) {CANErr_freq_root["BMS2_E_cell_deep_undervoltage"] = TJA_DATA.BMS_Error[1].E_cell_deep_undervoltage;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_dchg_overheat ) {CANErr_freq_root["BMS2_W_pkg_dchg_overheat"] = TJA_DATA.BMS_Error[1].W_pkg_dchg_overheat;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_dchg_overheat ) {CANErr_freq_root["BMS2_E_pkg_dchg_overheat"] = TJA_DATA.BMS_Error[1].E_pkg_dchg_overheat;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_dchg_undertemp ) {CANErr_freq_root["BMS2_W_pkg_dchg_undertemp"] = TJA_DATA.BMS_Error[1].W_pkg_dchg_undertemp;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_dchg_undertemp ) {CANErr_freq_root["BMS2_E_pkg_dchg_undertemp"] = TJA_DATA.BMS_Error[1].E_pkg_dchg_undertemp;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_dchg_overcurrent ) {CANErr_freq_root["BMS2_W_pkg_dchg_overcurrent"] = TJA_DATA.BMS_Error[1].W_pkg_dchg_overcurrent;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_dchg_overcurrent ) {CANErr_freq_root["BMS2_E_pkg_dchg_overcurrent"] = TJA_DATA.BMS_Error[1].E_pkg_dchg_overcurrent;}
            if ( TJA_DATA.BMS_Error[1].W_pkg_undervoltage ) {CANErr_freq_root["BMS2_W_pkg_undervoltage"] = TJA_DATA.BMS_Error[1].W_pkg_undervoltage;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_undervoltage ) {CANErr_freq_root["BMS2_E_pkg_undervoltage"] = TJA_DATA.BMS_Error[1].E_pkg_undervoltage;}
            if ( TJA_DATA.BMS_Error[1].E_VCU_COM ) {CANErr_freq_root["BMS2_E_VCU_COM"] = TJA_DATA.BMS_Error[1].E_VCU_COM;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_dchg_softstart ) {CANErr_freq_root["BMS2_E_pkg_dchg_softstart"] = TJA_DATA.BMS_Error[1].E_pkg_dchg_softstart;}
            if ( TJA_DATA.BMS_Error[1].E_dchg_relay_stuck ) {CANErr_freq_root["BMS2_E_dchg_relay_stuck"] = TJA_DATA.BMS_Error[1].E_dchg_relay_stuck;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_dchg_short ) {CANErr_freq_root["BMS2_E_pkg_dchg_short"] = TJA_DATA.BMS_Error[1].E_pkg_dchg_short;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_temp_diff ) {CANErr_freq_root["BMS2_E_pkg_temp_diff"] = TJA_DATA.BMS_Error[1].E_pkg_temp_diff;}
            if ( TJA_DATA.BMS_Error[1].E_cell_voltage_diff ) {CANErr_freq_root["BMS2_E_cell_voltage_diff"] = TJA_DATA.BMS_Error[1].E_cell_voltage_diff;}
            if ( TJA_DATA.BMS_Error[1].E_AFE ) {CANErr_freq_root["BMS2_E_AFE"] = TJA_DATA.BMS_Error[1].E_AFE;}
            if ( TJA_DATA.BMS_Error[1].E_MOS_overtemp ) {CANErr_freq_root["BMS2_E_MOS_overtemp"] = TJA_DATA.BMS_Error[1].E_MOS_overtemp;}
            if ( TJA_DATA.BMS_Error[1].E_external_EEPROM ) {CANErr_freq_root["BMS2_E_external_EEPROM"] = TJA_DATA.BMS_Error[1].E_external_EEPROM;}
            if ( TJA_DATA.BMS_Error[1].E_RTC ) {CANErr_freq_root["BMS2_E_RTC"] = TJA_DATA.BMS_Error[1].E_RTC;}
            if ( TJA_DATA.BMS_Error[1].E_ID_conflict ) {CANErr_freq_root["BMS2_E_ID_conflict"] = TJA_DATA.BMS_Error[1].E_ID_conflict;}
            if ( TJA_DATA.BMS_Error[1].E_CAN_msg_miss ) {CANErr_freq_root["BMS2_E_CAN_msg_miss"] = TJA_DATA.BMS_Error[1].E_CAN_msg_miss;}
            if ( TJA_DATA.BMS_Error[1].E_pkg_voltage_diff ) {CANErr_freq_root["BMS2_E_pkg_voltage_diff"] = TJA_DATA.BMS_Error[1].E_pkg_voltage_diff;}
            if ( TJA_DATA.BMS_Error[1].E_chg_dchg_current_conflict ) {CANErr_freq_root["BMS2_E_chg_dchg_current_conflict"] = TJA_DATA.BMS_Error[1].E_chg_dchg_current_conflict;}
            if ( TJA_DATA.BMS_Error[1].E_cable_abnormal ) {CANErr_freq_root["BMS2_E_cable_abnormal"] = TJA_DATA.BMS_Error[1].E_cable_abnormal;}

            // Serialize JSON Object to array of string
            char BMS2_CAN_ERR[measureJson(CANErr_freq_root) + 1];
            serializeJson(CANErr_freq_root, BMS2_CAN_ERR, measureJson(CANErr_freq_root) + 1);

            // Send data to Medium Frequency topic
            char BM2topic[sizeof(g_states.MQTTProject)+ sizeof(g_states.MQTTclientID)+ sizeof(g_states.MQTTCANErrFrequencyTopic) + 3];
            sprintf(BM2topic, "%s/%s/%s", g_states.MQTTProject, g_states.MQTTclientID, "BMS2_ERR");
            int BMS2_status = mqtt.publish(BM2topic, BMS2_CAN_ERR);

            if ( MCU_status && BMS1_status && BMS2_status ) return false;
            else return true; 
        };
};
CANErrorMQTT canErrorFrequency;