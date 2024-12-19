# Fire Alarm with Localized Mesh Technology and Auditorial Announcement (F.L.A.M.E.)
*Gold Medalist of Thailands SCiUS 14th Forum STEM branch*

![Units](https://github.com/ILFforever/F.L.A.M.E./blob/main/image/lineup.jpg "FLAME units")

## Project Overview

- *F.L.A.M.E.* is an innovative take on a fire alarm system that leverages **IoT and mesh networking** to provide accurate and efficient fire detection while being easy to setup for small and large scale usages. 
- Utalizing **WIFI mesh technology and TTS (Text to speech)**, FLAME provides a system of wirelessly interconnected alarms, equiped with verbal announcements, and public alert broadcasts for enhanced safety and communication.

## Features

### Accurate Fire Detection
- Utilizes an array 3 of **MQ-2** gas sensors and **DHT22** temperature sensors*
- for precise monitoring or **CO (Carbon monoxide)** and **Smoke particles**

![alt text](https://github.com/ILFforever/F.L.A.M.E./blob/main/image/top_off.JPG "Sensor service lid removed")

### Mesh Networking
- Implements **Wi-Fi mesh technology (via PainlessMesh)** for seamless 2-way communication between alarm units.

### Per-room Auditorial Alerts
- Generates verbal notifications using ESP-SAM (a speech synthesizer library for ESP32).
- Pioritizes reducing panic and confusion for occupants.
- Allow for PA announcements broadcasted wirelessly *(Via use of a Master terminal)*

![alt text](https://github.com/ILFforever/F.L.A.M.E./blob/main/image/bottom_compartment.jpg "Bottom_Compartment")
### Backup Power Supply
- FLAME. ensures functionality during power outages with a **built-in UPS solution providing 7200mah at 7.4 Volts**.
- Over **2.5 hours** of estimated battery life at the 3 Amps max load** 

### Plug and Play
- FLAME allows users to simply configure and power on the device. **FLAME will figure the rest!**
- No wiring or server required!

### Other Features
- FLAME. supports over 200 devices in a single mesh network, making it ideal for large buildings.
- Coming in at only **13 cm x 13 cm x 11 cm** each unit is compact and comes with a built-in AC-DC adapter!
- Loud piezo buzzer alert system for emergencies
- [Full presentation](https://www.canva.com/design/DAFgXDmPv9E/pQsmitHuM79DFmavD8RFxw/edit?utm_content=DAFgXDmPv9E&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton)
>   *DHT22 is only for monitoring MQ-2 heating element temperature
> 
>  **Before system enters low power mode

## Software
> Main Programming Languages: Arduino C++, Python

Main libraries Used:
```c
#include <ArduinoJson.h>
#include <Dictionary.h>
#include "DHTesp.h"
#include <EEPROM.h>
#include <ESP8266SAM.h>
#include "painlessMesh.h"
```

## Monitoring

Fire statuses and unit data can be viewed through either **remotely through a dashboard** if using the optional Wi-Fi module or **on site using the handheld master terminal**.

## Software Installation

Clone the repository:
```
git clone https://github.com/ILFforever/FLAME.git
```
- i Install required libraries via Arduino IDE or PlatformIO.
- ii Perform sensor Calibration and Flash EEPROM using *ESP-INJECT*
- iii Configure the mesh network settings in the config.h file.
- iiii Upload the provided sketches to the ESP32 boards.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments
*This project couldn't have been finished without advise and help from the following*
- Asst. Prof. Dr. Ratsameetip Wita 🔥
-  Mr. Peerapong Tuptim 🔥
-  Chiang Mai University for providing support and resources.

## Contact

For inquiries or collaboration, please contact:
Tanabodhi Mukura - Lead Developer
Email: hammymukura@gmail.com
