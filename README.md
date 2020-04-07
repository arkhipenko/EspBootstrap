# EspBootstrap

### A simple way to get your ESP8266 or ESP32 project configured and online quickly

#### Background

A typical journey of an IoT device on a first boot is:

1. Connect to WiFi network
2. Check for firmware update (download and install if available)
3. Connect to all the online services it has to connect to
4. Start doing whatever it was built to do

The problem is: how would a device know which WiFi to connect to? Or how would it know that configuration of the communication network has changed? 

That's when **EspBootstrap** comes to rescue:



#### **Concept**

EspBootstrap consists of three major components:

**Parameters** - a helper class that simplifies storing and loading configuration parameters in the EEPROM memory of a device. 

**EspBootstrap** - a helper class to quickly collect configuration parameter values from a user via simple web form in an Access Point mode

**JsonWebConfig** - a helper class to download and parse configuration file (simple JSON format) and populate respective configuration parameters 



#### Typical Implementation Journey

1. Define Parameters Structure

2. Define Parameter "defaults"

3. Describe Parameter structure layout for **EspBootstrap** and **JsonWebConfig**

4. Load or obtain configuration from a user and/or from the web

   

#### Typical device boot process

1. Attempt to load parameters from EEPROM (fails first time since nothing was ever saved)
2. Collect initial configuration parameters via web form (http://10.1.1.1) created by **EspBootstrap** (typically a WiFi SSID, password and a link to web-based configuration service)
3. Reboot and connect to WiFi with the recently obtained credentials
4. Load JSON configuration file from the web service using **JsonWebConfig** and populate respective configuration parameters
5. Save new configuration to EEPROM
6. Check for OTA update, download, install and reboot if update is available
7. Resume normal operation, periodically checking for OTA updates and config changes.







