/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This example illustrates how to use EspBootstrap library.
   The OTA update is not included.

   1. Compile and upload into the chip
   2. Start Serial monitor
   3. Boot up the chip for the first time
   4. Chip will create an Access Point names ESPNNN-XXXXXX
   5. Connect your phone/PC to that Access Point. There is no password
   6. Connect your browser to http://10.1.1.1. You have 5 minutes to do steps 7-9 below
   7. Populate the Wifi Seetings
   8. Populate the path to a valid JSON configuration file
   (e.g.,: http://raw.githubusercontent.com/arkhipenko/EspBootstrap/master/examples/EBS_example01/config.json)
   9. Press "Submit" button.
   10. Device will reboot
   11. Device should connect to your WiFi network, download, parse and save the configuration.

*/

// These two defines MUST be placed before the header files
// to be taken into account by the library
#define _USE_DICTIONARY_
#define  EEPROM_MAX 4096
#define CTOKEN  "EBS1"

#include <Parameters.h>
#include <EspBootstrap.h>
#include <JsonConfig.h>

#define _DEBUG_
//#define _TEST_

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif

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
const int NPARS_BTS = 3;

// Detect a WiFi connection timeout.
bool wifiTimeout;



// ==== CODE ==============================

// This methods prints out parameters for inspection
void printConfig() {
  _PL();
  _PL("Config dump");
  _PP("\ttoken : "); _PL(CTOKEN);
  _PP("\tssid  : "); _PL(d["ssid"]);
  _PP("\tpasswd: "); _PL(d["pwd"]);
  _PP("\tconfig: "); _PL(d["cfg_url"]);
  _PP("\tota host: "); _PL(d["ota_host"]);
  _PP("\tota port: "); _PL(d["ota_port"]);
  _PP("\tota url : "); _PL(d["ota_url"]);
  _PL();
}

int rc;

// Arduino SETUP method
void setup(void) {

  // Setting up Serial console
#ifdef _DEBUG_
  Serial.begin(115200);
  delay(500);
  {
    _PL("EspBootStrap Dict Example"); _PL();
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


  //  Pre-populate the dictionary with defaults for the webform
  //  First entry is a Web Form Title (element #0)
  d("Title", "EspBootstrapD");
  //  The rest will be created as fields on the form:
  d("ssid", "<wifi ssid>");
  d("pwd", "<wifi password>");
  d("cfg_url", "http://ota.home.lan/esp/config/ebs02config.json");

  // **** PARAMETERS COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // To work with Parameters we need to instanciate a Paramters object.
  //  first parameter  is the starting address in the EEPROM memory, {int}
  //  second parameter is a reference to the token, {String}
  //  third parameter  is a reference to the dictionary object holding parameters {Dictionary}
  //  fourth parameter is a size of EEPROM allocated to parameters
  //    since the combined size of all key-value pairs is not known, make sure you allocate
  //    enough space. Otherwise the Parameters object will not be activated.
  Parameters *p = new Parameters(0, TOKEN, d, 1024);

  // Since constructors do not return values, all error reporting is done with lastError() method
  // Calling lastError() right after initialization will indicate that EEPROM has enough memory
  // to hold your parameters.
  // Define EEPROM_MAX to explicitly set maximum EEPROM capacity for your chip
  int rc = p->lastError();
  _PP("EspBootStrap initialized. rc = "); _PL(rc);

  // load() methods attempts to load parameters from the EEPROM
  // It checks for a valid CRC and a match on the TOKENs.
  // If either CRC or token do not match, an error is set
  // load() can return OK, indicating that in the end the structure was populated with something,
  // however lastError() may indicate that values were overwritten with defaults due to CRC
  // or Token mismatch, and therefore return a non-zero value. Always check lastError()!
  // Upon success, the dictionary should be updated with key-value pairs from the EEPROM
  p->load();
  rc = p->lastError();
  _PP("Configuration loaded. rc = ");_PL(rc);
  printConfig();

  // If Parameters were loaded successfully, the device will try to
  // connect to the WiFi network (d["ssid"], d["pwd"]) specified in the configuration.
  // It will time out after 30 seconds on an assumption that WiFi config is invalid
  // 30 seconds is arbitrary - you can set it for longer of shorter period of time

  if (rc == PARAMS_OK) {
    _PL("Connecting to WiFi for 30 sec:");
    setupWifi();
    waitForWifi(30 * BOOTSTRAP_SECOND);
  }

  // If loading of parameters failed, or a WiFi connection timed out
  // we need to BootStrap the device.
  // ESPBootstrap is a static singleton object which will start up an Access Point (ESPXXX-NNNNNN),
  // and a webserver, and then display a simple web form as specified by the PAGE and PARS structures.
  // The webserver will be active for 5 minutes. This is done to prevent devices from going into
  // bootstrap mode forever due to intermittent WiFi issues. After 5 minutes, if the Submit button
  // was never pressed, the device will reboot and re-try to connect to current WiFi network.
  // This will continue until either the WiFi settings are changes on the web page, or WiFi network
  // issues with the current correct settings are resolved
  if (rc != PARAMS_OK || wifiTimeout) {
    _PL("Device needs bootstrapping:");


    // **** BOOTSTRAP COMPONENT OF ESPBOOTSTRAP LIBRARY ****
    // =====================================================
    // ESPBootstrap.run() takes 4 paramters:
    //  Number of parameters to display on the web form, {int}
    //  Reference to the dictionary object with title and fields, {Dictionary}
    //  Timeout in milliseconds. (can use helper constants BOOTSTRAP_MINUTE and BOOTSTRAP_SECOND)
    rc = ESPBootstrap.run(NPARS_BTS, d, 5 * BOOTSTRAP_MINUTE);

    if (rc == BOOTSTRAP_OK) {
      // If bootstrap was successful, new set of parameters should be saved,
      // and processing continued
      p->save();
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

  // **** JSONCONFIG COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // The rest of the parameters could be lifted off a JSON configuration file
  // Example of the JSON file is provided on the github.
  // JSON file rules:
  //  1. No arrays - just a list of key-value pairs
  //  2. All keys and values are strings and should be in quotation marks
  //  3. The order of key-value pairs in the file does not matter
  //  4. Keys and values can contain backslash-ed characters: e.g.; "key\"with a quote" : "\\path\\path2",
  //  5. Each line should end with a comma "," (except the last line)
  //
  // JSONConfig is a static singleton object that does HTTP call, parses the JSON file, and
  // populates respective configuration key-value pairs.
  // JSONConfig.parseHttp takes 3 parameters:
  //  a fully quallified URL pointing to a JSON configuation file (including http://) {char *}
  //  a reference to the parameter dictionary object, {Dictionary}
  // key-value pairs are ADDED or UPDATED in the dictionary, therefore JSON file may contain a subset of
  // parameters (e.g., the WiFi settings are always set by the Bootstrap web form, and the rest is
  // provided by the JSON file.
  rc = JSONConfig.parseHttp(d["cfg_url"], d);

  // If successful, the "d" dictionary should have a refreshed set of parameters from the JSON file.
  _PP("JSONConfig finished. rc = "); _PL(rc);
  _PP("Current dictionary count = "); _PL(d.count());
  _PP("Current dictionary size = "); _PL(d.size());
  printConfig();
  if (rc == 0) p->save();
}

void loop(void) {

}

// This method prepares for WiFi connection
void setupWifi() {
  _PP(millis()); _PL(": setup_wifi()");

  // We start by connecting to a WiFi network
  _PL("Connecting to WiFi...");
  // clear wifi config
  WiFi.disconnect();

  WiFi.mode(WIFI_STA);
  WiFi.begin(d["ssid"].c_str(), d["pwd"].c_str());
}

// This method waits for a WiFi connection for aTimeout milliseconds.
void waitForWifi(unsigned long aTimeout) {
  _PP(millis()); _PL(": waitForWifi()");

  unsigned long timeNow = millis();
  wifiTimeout = false;

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(1000);
    _PP(".");
    if ( millis() - timeNow > aTimeout ) {
      wifiTimeout = true;
      _PL(" WiFi connection timeout");
      return;
    }
  }

  _PL(" WiFi connected");
  _PP("IP address: "); _PL(WiFi.localIP());
  _PP("SSID: "); _PL(WiFi.SSID());
  _PP("mac: "); _PL(WiFi.macAddress());
}
