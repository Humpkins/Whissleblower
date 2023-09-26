#include <Arduino.h>

#define FS_NO_GLOBALS

#include "esp_spiffs.h"
#define SPIFFS_H

#include <ArduinoJson.h>
#include <stdio.h>
#include <stdint.h>

StaticJsonDocument<512> configRoot;

#define CONFIG_DATA

class states{
  private:
    int mountFailMax = 5;
    FILE *configFile;
    esp_err_t ret;

    char configFilePath[12] = "/config.txt";

  public:
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = true
    };

    char APN_GPRS [21] = "claro.com.br";
    char APN_CATM [21] = "cat-m1.claro.com.br";

    char APNUser [21] = "claro";
    char APNPassword [21] = "claro";

    bool VoltageInput = false;

    char MQTTProject [31] = "Whistleblower";
    char MQTTHost [40] = "test.mosquitto.org";//"0.tcp.sa.ngrok.io";
    int16_t MQTTPort = 1883;//17489;
    char MQTTclientID [21] = "MotoTesteTCC";
    char MQTTUsername [15] = "";//"Whistleblower";
    char MQTTPassword [20] = "";//"FB)TKqR#C@Jw7TTc";
    char MQTTInfoTopic [5] = "Info";
    char MQTTDataTopic [14] = "potenciometro";
    char MQTTTopic [31] = "Teste_moto_cliente_tcc";
    char mDNS[31] = "whistleblower";

    char MQTTWakeTopic [11] = "Wake";

    char MQTTLowFrequencyTopic [31] = "LowFrequency";
    int16_t MQTTLowPeriod = 1000;
    char MQTTMediumFrequencyTopic [31] = "MediumFrequency";
    int16_t MQTTMediumPeriod = 1000;
    char MQTTHighFrequencyTopic [31] = "HighFrequency";
    int16_t MQTTHighPeriod = 500;
    char MQTTCANErrFrequencyTopic [31] = "CANErrFrequency";
    int16_t MQTTCANErrPeriod = 7000;

    char MQTTOTATopic [4] = "OTA";

    char MQTTListenTopic [31] = "Are_u_talking_to_me?";
    char MQTTHeartBeatTopic [10] = "Heartbeat";

    char Ver [10] = "v@1.0.0.3";

    char HOST [21] = "whistleblower";

    char AP_SSID [21] = "TCU_TCC_Mateus";
    char AP_PSW [21] = "tcc_mateus";

    char STA_SSID [21] = "Unicon 2.4_Juliana";
    char STA_PSW [21] = "99061012";

    int MAX_CURRENT = 90;

    int N_BATERRIES = 0;

    float GEAR_RATIO = 1;
    float DRIVEN_WHEEL_MAX_DIAMETER_M = 0.6278;

  public:
  #ifdef SPIFFS_H
    bool saveConfigToSPIFFS(
      char * APN_GPRS, char * APNUser, char * APNPassword,
      char * MQTTHost, char * MQTTUsername, char * MQTTPassword,
      int MQTTPort, char * MQTTclientID, char * MQTTProject,
      char * STA_SSID, char * STA_PSW, char * AP_SSID, char * AP_PSW
    ){
      
      bool wasSerialOpened = true;
      if ( !Serial ) {
        wasSerialOpened = false;
        Serial.begin(115200);
      }

      // Checks for SPIFFS open
      this->ret = esp_vfs_spiffs_register(&this->conf);
      if ( this->ret != ESP_OK  ) return false;

      int attempt = 0;
      this->configFile = NULL;
      while (this->configFile == NULL) {
        attempt++;

        this->configFile = fopen( this->configFilePath, "w");

        if ( this->configFile == NULL ) continue;
        if ( attempt > this->mountFailMax ) {
          Serial.println("    [FAIL]");
          Serial.println("Error on saving the confguration file: The file wasn't open");
          esp_vfs_spiffs_unregister(conf.partition_label);
          
          if ( !wasSerialOpened ) Serial.end();

          return false;
        }

      }

      Serial.print("Saving configuration data to SPIFFS");

      strcpy(this->APN_GPRS,      APN_GPRS);
      strcpy(this->APNUser,       APNUser);
      strcpy(this->APNPassword,   APNPassword);
      strcpy(this->MQTTHost,      MQTTHost);
      strcpy(this->MQTTUsername,  MQTTUsername);
      strcpy(this->MQTTPassword,  MQTTPassword);
      this->MQTTPort =            MQTTPort;
      strcpy(this->MQTTclientID,  MQTTclientID);
      strcpy(this->MQTTProject,   MQTTProject);
      strcpy(this->STA_SSID,      STA_SSID);
      strcpy(this->STA_PSW,       STA_PSW);
      strcpy(this->AP_SSID,       AP_SSID);
      strcpy(this->AP_PSW,        AP_PSW);

      configRoot.clear();

      configRoot["APN_GPRS"] =          this->APN_GPRS;
      configRoot["APNUser"] =           this->APNUser;
      configRoot["APNPassword"] =       this->APNPassword;
      configRoot["MQTTHost"] =          this->MQTTHost;
      configRoot["MQTTUsername"] =      this->MQTTUsername;
      configRoot["MQTTPassword"] =      this->MQTTPassword;
      configRoot["MQTTPort"] =          this->MQTTPort;
      configRoot["MQTTclientID"] =      this->MQTTclientID;
      configRoot["MQTTProject"] =       this->MQTTProject;
      configRoot["STA_SSID"] =          this->STA_SSID;
      configRoot["STA_PSW"] =           this->STA_PSW;
      configRoot["AP_SSID"] =           this->AP_SSID;
      configRoot["AP_PSW"] =            this->AP_PSW;

      //  Serialize JSON Object to array of string
      char configBuffer[measureJson(configRoot) + 1];
      serializeJson(configRoot, configBuffer, measureJson(configRoot) + 1);
        
      if ( fprintf(this->configFile, configBuffer ) < 0 ){
        Serial.println("    [FAIL]");
        Serial.println("Error on saving the configuration file: It wasn't possible to write the configuration on the file");
        return false;
      }

      fclose( this->configFile );
      esp_vfs_spiffs_unregister(conf.partition_label);

      Serial.println("    [OK]");
      if ( !wasSerialOpened ) Serial.end();

      return true;

    }
  #endif

  #ifdef SPIFFS_H

    void writeDefaultConfigurations() {

      configRoot.clear();

      configRoot["APN_GPRS"] =          this->APN_GPRS;
      configRoot["APNUser"] =           this->APNUser;
      configRoot["APNPassword"] =       this->APNPassword;
      configRoot["MQTTHost"] =          this->MQTTHost;
      configRoot["MQTTUsername"] =      this->MQTTUsername;
      configRoot["MQTTPassword"] =      this->MQTTPassword;
      configRoot["MQTTPort"] =          this->MQTTPort;
      configRoot["MQTTclientID"] =      this->MQTTclientID;
      configRoot["MQTTProject"] =       this->MQTTProject;
      configRoot["STA_SSID"] =          this->STA_SSID;
      configRoot["STA_PSW"] =           this->STA_PSW;
      configRoot["AP_SSID"] =           this->AP_SSID;
      configRoot["AP_PSW"] =            this->AP_PSW;

      //  Serialize JSON Object to array of string
      char configBuffer[measureJson(configRoot) + 1];
      serializeJson(configRoot, configBuffer, measureJson(configRoot) + 1);

      // Checks for SPIFFS open
      this->ret = esp_vfs_spiffs_register(&this->conf);
      if ( this->ret != ESP_OK  ) return;

      int attempt = 0;
      this->configFile = NULL;
      while (this->configFile == NULL) {
        attempt++;

        this->configFile = fopen( this->configFilePath, "w");

        if ( this->configFile == NULL ) continue;
        if ( attempt > this->mountFailMax ) {
          Serial.println("    [FAIL]");
          Serial.println("Error on saving the confguration file: The file wasn't open");
          esp_vfs_spiffs_unregister(conf.partition_label);

          return;
        }

      }

      if ( fprintf(this->configFile, "w") < 0 ) {
        Serial.println("    [FAIL]");
        Serial.println(
          "Error on saving the configuration file: It wasn't possible to write the configuration on the file"
        );
        return;
      };

      fclose( this->configFile );
      esp_vfs_spiffs_unregister(conf.partition_label);

      Serial.println("    [OK]");

    }

    bool loadConfigFromSPIFFS(){
        
      bool wasSerialOpened = true;
      if ( !Serial ) {
        wasSerialOpened = false;
        Serial.begin(115200);
      }

      Serial.print("Retrieving configuration data from SPIFFS\n");

      // Checks for SPIFFS open
      this->ret = esp_vfs_spiffs_register(&this->conf);
      if ( this->ret != ESP_OK  ) return false;

      int attempt = 0;
      this->configFile = NULL;
      while (this->configFile == NULL) {
        attempt++;

        this->configFile = fopen( this->configFilePath, "r");

        if ( this->configFile == NULL ) continue;
        if ( attempt > this->mountFailMax ) {
          Serial.println("    [FAIL]");
          Serial.print("There was no configFile on the flash memory.\nCreating one with default settings and continuing the program");
          esp_vfs_spiffs_unregister(conf.partition_label);

          this->writeDefaultConfigurations();
        }

      }

      Serial.println("    [OK]");

      //  Read the file into local variable
      unsigned int fileSize = 500;
      char configJSONbuffer[ fileSize + 1];
      int loop_read = 0;

      fgets( configJSONbuffer, fileSize, this->configFile );
      char* pos = strchr( configJSONbuffer, '\n' );
      if ( pos ) *pos = '\0';

      Serial.print("Loading config file");
      configRoot.clear();

      DeserializationError error = deserializeJson(configRoot, configJSONbuffer);
      if (error) {
        Serial.println("    [FAIL]");
        Serial.printf("Data retrieved is not in correct format:\n %s", configJSONbuffer);
        utilities.ESPReset();
      }

      strcpy( this->APN_GPRS, configRoot["APN_GPRS"]);
      strcpy( this->APNUser, configRoot["APNUser"]);
      strcpy( this->APNPassword, configRoot["APNPassword"]);
      strcpy( this->MQTTHost, configRoot["MQTTHost"]);
      strcpy( this->MQTTUsername, configRoot["MQTTUsername"]);
      strcpy( this->MQTTPassword, configRoot["MQTTPassword"]);
      this->MQTTPort = configRoot["MQTTPort"];
      strcpy( this->MQTTclientID, configRoot["MQTTclientID"]);
      strcpy( this->MQTTProject, configRoot["MQTTProject"]);
      strcpy( this->STA_SSID, configRoot["STA_SSID"]);
      strcpy( this->STA_PSW, configRoot["STA_PSW"]);
      strcpy( this->AP_SSID, configRoot["AP_SSID"]);
      strcpy( this->AP_PSW, configRoot["AP_PSW"]);
      strcpy( this->mDNS, configRoot["mDNS"]);

      fclose(this->configFile);
      esp_vfs_spiffs_unregister(conf.partition_label);

      Serial.println("    [OK]");
      if ( !wasSerialOpened ) Serial.end();

      Serial.printf(
          "APN_GPRS: %s\nAPNUser: %s\nAPNPassword: %s\nMQTTHost: %s\nMQTTUsername: %s\nMQTTPassword: %s\nMQTTclientID: %s\nMQTTProject: %s\nMQTTPort: %i\nSTA_SSID: %s\nSTA_PSW: %s\nAP_SSID: %s\nAP_PSW: %s\n",
          this->APN_GPRS,
          this->APNUser,
          this->APNPassword,
          this->MQTTHost,
          this->MQTTUsername,
          this->MQTTPassword,
          this->MQTTclientID,
          this->MQTTProject,
          this->MQTTPort,
          this->STA_SSID,
          this->STA_PSW,
          this->AP_SSID,
          this->AP_PSW
      );

      return true;
  }
  #endif
};

states g_states;
