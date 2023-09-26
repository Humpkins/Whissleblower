    #include <stdio.h>
    #include <string.h>
    #include <sys/unistd.h>
    #include <sys/stat.h>

    #include "esp_system.h"
    #include "esp_spi_flash.h"
    #include "esp_log.h"
    #include "esp_vfs_fat.h"
    #include "driver/gpio.h"
    #include "driver/spi_master.h"
    #include "sdmmc_cmd.h"

    #define SCK GPIO_NUM_14
    #define MISO GPIO_NUM_2
    #define MOSI GPIO_NUM_15
    #define SS GPIO_NUM_13

    #define MOUNT_POINT "/sdcard"

    class SDCard {
        private:
            int16_t mountFailMax = 5;
            esp_err_t ret;

        public:

            int mountSD(){

                int wasSerialOpen = 0;
                if ( Serial ) {
                    wasSerialOpen = 1;
                    Serial.end();
                }

                // Options for mounting the filesystem.
                // If format_if_mount_failed is set to true, SD card will be partitioned and
                // formatted in case when mounting fails.
                const esp_vfs_fat_mount_config_t mount_config = {
                    .format_if_mount_failed = false,
                    .max_files = 10,
                    .allocation_unit_size = 16 * 1024
                };

                sdmmc_card_t *card;

                sdmmc_host_t host = SDSPI_HOST_DEFAULT();
                spi_bus_config_t bus_cfg = {
                    .mosi_io_num = MOSI,
                    .miso_io_num = MISO,
                    .sclk_io_num = SCK,
                    .quadwp_io_num = -1,
                    .quadhd_io_num = -1,
                    .max_transfer_sz = 2000,
                };

                ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);

                sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
                slot_config.gpio_cs = SS;
                slot_config.host_id = SPI2_HOST;


                for ( int i = 0; i < this->mountFailMax; i++ ) {

                    // Initialize the SD card
                    this->ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
                    
                    if ( wasSerialOpen ) Serial.begin(115200);
                    while( !Serial );

                    if  ( this->ret == ESP_ERR_INVALID_STATE ) return 1;
                    if  ( this->ret == ESP_OK ) return 1;

                    vTaskDelay( 750 / portTICK_PERIOD_MS );

                }

                if ( wasSerialOpen ) Serial.begin(115200);
                while( !Serial );

                return 0;

            }

            int unmountSD(){
                esp_vfs_fat_sdmmc_unmount();
                return 1;
            }

            void setup(){
                //  If the card is not mounted
                if ( this->mountSD() ) Serial.println("[OK]     SD mount compleated");
                else Serial.println(
                    "[ERROR]       Error on mounting SD card. Check if SD card is properlly placed and try again"
                );
            }

    };

    SDCard sd_card;