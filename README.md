# Whistleblower
This project aims to develop a Telemetry Control Unit (TCU) using some knowledges in GSM, LTE, GNSS and MQTT.
<br />for the front end interface development, please refer to [Whistleblower Frontend](https://github.com/Humpkins/Whissleblower_Frontend)

This device is capable of intercept the vehicle comunication (CAN) and send some key metrics to a remote server (MQTT). The Front-End software then retrieve this information (WS) and displays on an user-friedly web page.

![1693487001317](https://github.com/Humpkins/Whistleblower/assets/50257157/eb34f7ab-cdd8-463e-8173-a05086b34b67)

<br />The landing page. Here you can see all the connected devices and choose which one you want to see the details
![Group 65 (2)](https://github.com/Humpkins/Whistleblower/assets/50257157/08f10123-6de5-41a2-b9dd-bfe77aec145d)
<br /><br /><br />On the detail page, you can check the states of the vehicle, as well as start/stop the datalogging service
![Group 62 (2)](https://github.com/Humpkins/Whistleblower/assets/50257157/c43104bc-8864-4c87-a41c-7459c6ec7e8b)
<br /><br /><br />If the user wants to see/change the device settings, he can enable (on the landing page) the access/station wifi mode to access it on any device (notebook/smartphone). The user can also manage the logged data through this interface.
![Group 72 (1)](https://github.com/Humpkins/Whistleblower/assets/50257157/0cef7e6a-6063-4f0a-86a5-4bcedf411836)


| Functionality                                       | Identification |
|------------------------------------------------------|-------------- |
| Online devices in real time                         | 1            |
| Inertial state                                      | 2            |
| Powertrain                                          | 3            |
| Batteries                                           | 4            |
| Geopositioning                                      | 5            |
| Mobile network                                      | 6            |
| Component's error log                               | 7            |
| Datalogger                                          | 8            |
| Internal settings configuration                     | 9            |
| Download logged data                                | 10           |
| Delete logged data                                  | 11           |

https://github.com/Humpkins/Whistleblower/assets/50257157/a2c75d3d-6d58-4009-bb95-e40281f73e24

It was designed a aditional hardware so it can interface the ESP32 to +64V power source. It was used an XL7015 to source power from the vehicle's battery. For the vehicle comunication interface, it was used an TJA1050 CAN transceiver togheter with the ESP's internal SJA1000 CAN controller. It was also integrated an MPU6050 for inertial measurements.
![Group 136](https://github.com/Humpkins/Whistleblower/assets/50257157/80e73846-bbca-4a4a-99ca-bc6edd551c5e)

Since LILYGO provides an plastic case design for the ESP32 SIM7000G, it was only needed to extend the upper part in order to integrate the aditional designed hardware.
![Group 137](https://github.com/Humpkins/Whistleblower/assets/50257157/9b073c15-de47-44ab-b08f-663a11d136ca)

The software components were designed over the telemetry hardware to create tasks around the shared resources. It was also created a storage class so all the acquired data could be stored in a cashed memory for fast read/write operations.
![Group 138](https://github.com/Humpkins/Whistleblower/assets/50257157/01a7036b-c9e8-4b7d-b71b-e5056d996514)

The cellular and GNS data had the following flux into the hardware
![Group 139](https://github.com/Humpkins/Whistleblower/assets/50257157/65b7561b-0d85-4227-ab1a-0fee23e0210e)

The wifi data has the following flux into the hardware
![Group 140](https://github.com/Humpkins/Whistleblower/assets/50257157/1307f2fb-aff0-4740-93bb-db68c001f3b7)
