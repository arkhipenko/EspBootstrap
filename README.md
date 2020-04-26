# EspBootstrap 2

### A simple way to get your ESP8266 or ESP32 project configured and online quickly

#### Background

A typical journey of an IoT device on a first boot is:

1. Connect to WiFi network
2. Check for firmware update (download and install if available)
3. Connect to all the online services it has to connect to
4. Start doing whatever it was built to do

The problem is: how would a device know which WiFi to connect to? Or how would it know that configuration of the communication network has changed? 

That's when **EspBootstrap** comes to rescue: you can quickly provision initial configuration parameters, get your device connected to the WiFi, load, parse and store to EEPROM full set of configuration parameters, and be ready for doing whatever amazing things you plan to do.



### **Concept**

**EspBootstrap** consists of three major components:

**Parameters** - a helper class that simplifies storing and loading configuration parameters in the EEPROM memory or SPIFFS filesystem of a device. 

**EspBootstrap** - a helper class to quickly collect configuration parameter values from a user via simple web form in an Access Point mode

**JsonConfig** - a helper class to download/read and parse configuration file (simple JSON format) and populate respective configuration parameters 

Both direct structure mapping and use of [Dictionary](https://github.com/arkhipenko/Dictionary.git) data type is supported. Using dictionary is a simpler way, but may not be suitable for AVR devices with small memory. 

Dictionary approach is the default. 



## DICTIONARY APPROACH:

#### Typical Implementation Journey

(see [example #2](https://github.com/arkhipenko/EspBootstrap/blob/master/examples/EBS_example02/EBS_example02.ino))

1.  Create a Dictionary object 
2. Pass dictionary to appropriate **ESPBootstrap** and **JSONConfig** objects to load or obtain configuration from a user, device filesystem and/or from the web



#### Typical device boot process

1. Attempt to load parameters from EEPROM or file (fails first time since nothing was ever saved)
2. Collect initial configuration parameters via web form (http://10.1.1.1) created by **ESPBootstrap** (typically a WiFi SSID, password and a link to web-based configuration service)
3. Reboot and connect to WiFi with the recently obtained credentials
4. Load JSON configuration file from the web service or filesystem using **JSONConfig** and populate respective configuration parameters into the provided dictionary object
5. Save new configuration to EEPROM or file
6. Check for OTA update, download, install and reboot if update is available
7. Resume normal operation, periodically checking for OTA updates and config changes.





## STRUCTURE MAPPING APPROACH: 

#### Typical Implementation Journey 

(see [example #1](https://github.com/arkhipenko/EspBootstrap/blob/master/examples/EBS_example01/EBS_example01.ino))

1. Define Parameters Structure

2. Define Parameter "defaults"

3. Describe Parameter structure layout for **ESPBootstrap** and **JSONConfig** 

4. Load or obtain configuration from a user and/or from the web

   

#### Typical device boot process

1. Attempt to load parameters from EEPROM/filesystem (fails first time since nothing was ever saved)
2. Collect initial configuration parameters via web form (http://10.1.1.1) created by **EspBootstrap** (typically a WiFi SSID, password and a link to web-based configuration service)
3. Reboot and connect to WiFi with the recently obtained credentials
4. Load JSON configuration file from the web service or a file using **JsonWebConfig** and populate respective configuration parameters
5. Save new configuration to EEPROM or to the filesystem
6. Check for OTA update, download, install and reboot if update is available
7. Resume normal operation, periodically checking for OTA updates and config changes.



## Object Types

Types of **Parameters**, **JsonConfig** and **EspBootstrap** objects are chosen based on where you want to store your configuration and where you want to source your updates from:



#### Dictionary-based parameters, EEPROM storage, HTTP update

```c++
#include <ParametersEEPROM.h>
#include <EspBootstrapDict.h>
#include <JsonConfigHttp.h>
```

See examples #2 and #3 for implementation details. 



#### Memory structure-based parameters, EEPROM storage, HTTP update

```C++
#include <ParametersEEPROMMap.h>
#include <EspBootstrapMap.h>
#include <JsonConfigHttpMap.h>
```

See example #1 for implementation details. 



#### Dictionary-based parameters, SPIFFS filesystem, storage

```C++
#include <ParametersSPIFFS.h>
#include <EspBootstrapDict.h>
```

Parameters object uses **JsonConfig** file load/save capability to store the parameters. 

See examples #4 and #5 for implementation details. 



#### Full list of possible object types:

```C++
EspBootstrapDict
EspBootstrapMap

ESPBootstrap - static object to use for processing (correct type is assigned)
```

```C++
JsonConfigHTTP
JsonConfigHTTPMap
JsonConfigSPIFFS
JsonConfigSPIFFSMap
    
JSONConfig - static object to use for processing (correct type is assigned)
```

```C++
ParametersEEPROM
ParametersEEPROMMap
ParametersSPIFFS
```

If additional storage or update types are required, they could be implemented later (e.g., ParametersSD or JsonConfigFTP)



## ERROR CODES:

### Parameters:

```
#define PARAMS_OK   0
#define PARAMS_ERR  (-1)
#define PARAMS_LEN  (-2)
#define PARAMS_CRC  (-3)
#define PARAMS_TOK  (-4)
#define PARAMS_MEM  (-98)
#define PARAMS_ACT  (-99)
```



`PARAMS_OK`    - operation finished successfully

`PARAMS_ERR`  - operation finished with an error, unspecified 

`PARAMS_LEN`  - requested size or EPROM size is not sufficient to store parameters

`PARAMS_CRC`  - data inconsistency between parameters in memory and in EEPROM

`PARAMS_TOK`  - parameter tokens do not match

`PARAMS_MEM`  - failed to allocate memory for parameters buffer

`PARAMS_ACT`  - parameters engine was not activated with `begin()` method (not allocated)



### EspBootstrap:

```
#define BOOTSTRAP_OK        0
#define BOOTSTRAP_ERR      (-1)
#define BOOTSTRAP_TIMEOUT (-99)
```



`BOOTSTRAP_OK`	- bootstrap was successful. Parameters were entered and stored. No timeouts. 

`BOOTSTRAP_ERR`  - bootstrap process ended with errors (e.g., webserver failed to initiate)

`BOOTSTRAP_TIMEOUT`	- bootstrap process ran out of time waiting for user inputs. 



### JsonConfig:

```
#define JSON_OK         0
#define JSON_ERR      (-1)
#define JSON_COMMA    (-2)
#define JSON_COLON    (-3)
#define JSON_QUOTE    (-4)
#define JSON_BCKSL    (-5)
#define JSON_HTTPERR  (-97)
#define JSON_NOWIFI   (-98)
#define JSON_EOF      (-99)
```



`JSON_OK`         - operation finished successfully. All fields were processed as expected. 

`JSON_ERR`      - operation finished with unspecified error (reserved for future use and/or testing)

`JSON_COMMA`    - missing comma. (Detected ":" before detecting a value)

`JSON_COLON`    - missing colon. (Detected a comma or a new line before detecting a value)

`JSON_QUOTE`   - missing closing quotation mark

`JSON_BCKSL`    - orphaned back-slash. Back-slash not followed by a "verbatim" character. 

`JSON_HTTPERR`  - general HTTP error. Cannot initiate a connection to provided URL. 

`JSON_NOWIFI`   - device is not connected to WiFi

`JSON_EOF`      - unexpected end of file. 

**JsonConfig** also returns all HTTP error codes in case of HTTP GET operation failure. 
For instance: `404` - page not found. 



------

###### EspBootstrap v1  is deprecated, but still included in the library for backwards compatibility.