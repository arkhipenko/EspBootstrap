/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This example illustrates how to use EspBootstrap library.
   The OTA update is not included.

   1. Compile and upload into the chip
   2. Start Serial monitor
   3. Boot up the chip for the first time
   4. Chip will attempt to load parameters and connect to WiFi
   5. If not successful - chip will create an Access Point names ESPNNN-XXXXXX
      a. Connect your phone/PC to that Access Point. There is no password
      b. Connect your browser to http://10.1.1.1. You have 5 minutes to do steps 7-9 below
      c. Populate the Wifi Seetings
      d. Populate the host/port/url to a valid JSON configuration file
      (e.g.,: raw.githubusercontent.com, 80, /arkhipenko/EspBootstrap/master/examples/EBS_example01/config.json)
      e. Press "Submit" button.
      f. Device will reboot
   6. Device should connect to your WiFi network, download, parse and save the configuration
   7. If successful, device will remember that at least one successful attempt to download configuration was made,
       and going forward this set could be used offline if configuration server suddenly goes offline.
       If configurations were never downloaded successfully ata least once, then the

*/

// These two defines MUST be placed before the header files
// to be taken into account by the library
#define  EEPROM_MAX 4096
#define CTOKEN  "EBS3b"

// ====================================
#define SSID1 "YOUR WIFI SSID HERE"
#define PWD1  "YOUR WIFI PASSWORD HERE"

#define _DEBUG_
//#define _TEST_

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif


// ==== INCLUDES ======================
#include <ParametersEEPROM.h>
#include <EspBootstrapDict.h>
#include <JsonConfigHttp.h>


// ==== Parameters and BootStrap ===================

// Every configration structure should start with a token
// "Token" is a character string identifying set of parameters (and a version)
const String TOKEN(CTOKEN);

// NPARS is a number of parameters in the structure
// inclusive of the Token
const int NPARS = 7;

// The actual dictionary object to hold parameter values
Dictionary d(NPARS);

// The next two variables define how many fields are displayed on the web form
// when the form is constructed. You could include all fields, or just a subset
// First line is used as a title, the rest are the field labels
// This structure works in conjunction with dictionary. Only the first NPARS_BTS
// fields are displayed on the form. Fields are numbered in the order they inserted.
const int NPARS_BTS = 5;

// ==== CODE ==============================

// This methods prints out parameters for inspection
void printConfig() {
  _PL();
  _PL("Config dump");
  _PP("\ttoken : "); _PL(CTOKEN);
  _PP("\tssid  : "); _PL(d["ssid"]);
  _PP("\tpasswd: "); _PL(d["pwd"]);
  _PP("\tcfg host: "); _PL(d["cfg_host"]);
  _PP("\tcfg port: "); _PL(d["cfg_port"]);
  _PP("\tcfg url: ");  _PL(d["cfg_url"]);
  _PP("\tota host: "); _PL(d["ota_host"]);
  _PP("\tota port: "); _PL(d["ota_port"]);
  _PP("\tota url : "); _PL(d["ota_url"]);
  _PL();
}



// Arduino SETUP method
void setup(void) {
  int rc;            // Return code of the operations
  bool wifiTimeout;  // Detect a WiFi connection timeout.

  // Setting up Serial console
#ifdef _DEBUG_
  Serial.begin(115200);
  delay(500);
  {
    _PL("EspBootStrap Alternative Dict Example"); _PL();
#if defined( ARDUINO_ARCH_ESP8266 )
    String id = "ESP8266-" + String(ESP.getChipId(), HEX);
#endif
#if defined( ARDUINO_ARCH_ESP32 )
    String id = "ESP32-" + String((uint32_t)( ESP.getEfuseMac() & 0xFFFFFFFFL ), HEX);
#endif
    _PP("ESP Chip ID: "); _PL(id);
    _PL();
  }
#endif


  // =====================================================
  // Alternative approach of bootstrapping for the case where
  // some of the key parameters' defaults maybe known at the
  // time of coding.
  // The benefit of doing it this way is - the device maybe able to 
  // connect to a default wifi network and update its parameters to 
  // the latest set in an unattended way without a need for manual
  // bootstrapping. 
  
  // STEP 1: The default wifi connection is attempted, and device will
  // enter bootstrap mode if wifi connection could not be established

  //  First entry is a Web Form "Title" (element #0)
  d("Title", "EspBootstrap Alt");
  //  The rest will be created as fields on the form:
  d("ssid", SSID1);
  d("pwd", PWD1);
  d("cfg_host", "ota.home.lan");
  d("cfg_port", "80");
  d("cfg_url", "/esp/config/ebs03b_config.json");

  // **** PARAMETERS COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // To work with Parameters we need to instanciate a Paramters object.
  //  first parameter is a reference to the token, {String}
  //  second parameter  is a reference to the dictionary object holding parameters {Dictionary}
  //  third parameter  is the starting address in the EEPROM memory, {int}
  //  fourth parameter is a size of EEPROM allocated to parameters
  //    since the combined size of all key-value pairs is not known, make sure you allocate
  //    enough space. Otherwise the Parameters object will not be activated.
  ParametersEEPROM *p_ptr = new ParametersEEPROM(TOKEN, d, 0, 1024);
  ParametersEEPROM& p = *p_ptr;

  // Define EEPROM_MAX to explicitly set maximum EEPROM capacity for your chip
  rc = p.begin();
  _PP("EspBootStrap ParametersEEPROM initialized. rc = "); _PL(rc);

  // load() methods attempts to load parameters from the EEPROM
  // It checks for a valid CRC and a match on the TOKENs.
  // If either CRC or token do not match, an error is set
  // load() rc may indicate that values were overwritten with defaults due to CRC
  // or Token mismatch, and therefore return a non-zero value. Always check return code!
  // Upon success, the dictionary should be updated with key-value pairs from the EEPROM
  rc = p.load();
  _PP("Configuration loaded. rc = "); _PL(rc);
  printConfig();

  // The device will try to connect to the WiFi network (d["ssid"], d["password"])
  // specified in the configuration, which could be part of the saved parameters or defaults.
  // It will time out after 30 seconds on an assumption that WiFi config is invalid
  // 30 seconds is arbitrary - you can set it for longer of shorter period of time

  _PP("Connecting to WiFi for 30 sec:");
  setupWifi(d["ssid"].c_str(), d["pwd"].c_str());
  wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);

  // STEP 2:
  // If connected to WiFi, device will attempt to load parameters from the configuration server
  // This sketch uses the option with explicit host/port/url schema:
  // NOTE: host should not have 'http://' in front, just the domain name or ip address
  if ( !wifiTimeout ) {
    rc = JSONConfig.parse( d["cfg_host"], d["cfg_port"].toInt(), d["cfg_url"], d);
    // If successful, the "d" dictionary should have a refreshed set of parameters from the JSON file.
    _PP("JSONConfig finished. rc = "); _PL(rc);
    _PP("Current dictionary count = "); _PL(d.count());
    _PP("Current dictionary size = "); _PL(d.size());

    // If reading configration JSON file ended successfully, a special key-value pair ("saved ok")
    // is inserted into the dictionary. This key indicated that the device was able to read configuration
    // successfully at least once, and can use this configuration for normal operations even if
    // subsequent configuration read is not successful for whatever reason.
    // This allows device to continue to operate even if, for instance, the configuration server is down.
    if (rc == JSON_OK) {
      p.save();
      d("saved", "ok");
    }
  }

  // STEP 3:
  // Now we are ready to assess what happened with our attempt to connect to WiFi,
  // load existing parameters, and parse JSON configuration from the config server. 
  // The device will need bootstrapping in case of:
  //   1. WiFi timed out  -OR-
  //   2. JSON config parsing ended up in error and a good set of parameters was never saved 
  if ( wifiTimeout || !( rc == JSON_OK || d("saved") ) ) {
    _PL("Device needs bootstrapping:");

    // **** BOOTSTRAP COMPONENT OF ESPBOOTSTRAP LIBRARY ****
    // =====================================================
    // ESPBootstrap.run() takes 3 paramters:
    //  Reference to the dictionary object with title and fields, {Dictionary}
    //  Number of parameters to display on the web form, {int}
    //  Timeout in milliseconds. (can use helper constants BOOTSTRAP_MINUTE and BOOTSTRAP_SECOND)
    rc = ESPBootstrap.run(d, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);

    if (rc == BOOTSTRAP_OK) {
      // If bootstrap was successful, new set of parameters should be saved,
      // and processing continued
      p.save();
      _PL("Bootstrapped OK. Rebooting.");
    }
    else {
      _PL("Bootstrap timed out. Rebooting.");
    }
    // or device should be restarted after a timeout
    printConfig();
    delay(5000);
    ESP.restart();
  }

  // If you got this far you should have configuration loaded and ready to go.
  _PP("Current dictionary count = "); _PL(d.count());
  _PP("Current dictionary size = "); _PL(d.size());
  printConfig();

  // Continue with other setup() activities
}

void loop(void) {
}

// This method prepares for WiFi connection
void setupWifi(const char* ssid, const char* pwd) {
  _PL("Setup_wifi()");

  // We start by connecting to a WiFi network
  _PL("Connecting to WiFi...");
  // clear wifi config
  WiFi.disconnect();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
}

// This method waits for a WiFi connection for aTimeout milliseconds.
bool waitForWifi(unsigned long aTimeout) {
  _PL("WaitForWifi()");

  unsigned long timeNow = millis();

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(1000);
    _PP(".");
    if ( millis() - timeNow > aTimeout ) {
      _PL(" WiFi connection timeout");
      return true;
    }
  }

  _PL(" WiFi connected");
  _PP("IP address: "); _PL(WiFi.localIP());
  _PP("SSID: "); _PL(WiFi.SSID());
  _PP("mac: "); _PL(WiFi.macAddress());
  return false;
}
