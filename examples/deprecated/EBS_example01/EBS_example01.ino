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
// IMPORTANT: Token should be the fist element of the parameters structure.
#define CTOKEN  "EBS2"
const String TOKEN(CTOKEN);

// NPARS is a number of parameters in the structure
// inclusive of the Token
const int NPARS = 7;

// The parameter structure itself defined as a new type
// All parameters shoudl be of type 'char[]'
// Take care to allocate enough space for your parameters
// There is no buffer overrun checking done by this library
// (for size and speed perposes)
typedef struct {
  char token[5];
  char ssid[32];
  char pwd[32];
  char cfg_url[128];
  char ota_host[32];
  char ota_port[6];
  char ota_url[32];
} Params;

// Define actual parameter variable
Params eg;

// You can supply default values for the parameters
// Which will be loaded into the structure in case
// of bad crc or token mismatch
// You can provide NULL pointer instead of Defaults
// In that case the sctructure will be filled with zeros
Params defaults = { CTOKEN,
                    "<wifi ssid>",
                    "<wifi password>",
                    //                  "http://raw.githubusercontent.com/arkhipenko/EspBootstrap/master/examples/EBS_example01/config.json",
                    "http://ota.home.lan/esp/config/ebs01config.json",
                    "<ota.server.com>",
                    "1234",
                    "/esp/ota.php"
                  };

// A very important part of the process is a map of parameter fields
// This map is used by many library components to populate individual fields.
// The map SHOULD NOT include reference to the TOKEN
char* PARS[] = { eg.ssid,
                 eg.pwd,
                 eg.cfg_url,
                 eg.ota_host,
                 eg.ota_port,
                 eg.ota_url
               };

// The next two variables define how many fields are displayed on the web form
// when the form is constructed. You could include all fields, or just a subset
// First line is used as a title, the rest are the field labels
// This structure works in conjunction with PARS[] structure to identify
// which fields to actually populate.
const int NPARS_BTS = 3;
const char* PAGE[] = { "EspBootstrap",
                       "WiFi SSID",
                       "WiFi PWD",
                       "Config URL",
                     };

// Boolean variable indicating that attempt to connect to WiFi ended in a timeout


// ==== CODE ==============================

// This methods prints out all parameters for inspection
void printConfig() {
  _PL();
  _PL("Config dump");
  _PP("\ttoken    : "); _PL(eg.token);
  _PP("\tssid     : "); _PL(eg.ssid);
  _PP("\tpasswd   : "); _PL(eg.pwd);
  _PP("\tconfig   : "); _PL(eg.cfg_url);
  _PP("\tota host : "); _PL(eg.ota_host);
  _PP("\tota port : "); _PL(eg.ota_port);
  _PP("\tota url  : "); _PL(eg.ota_url);
  _PL();
}


// Arduino SETUP method
void setup(void) {
  bool wifiTimeout;
  int rc;
  
  // Setting up Serial console
#ifdef _DEBUG_
  Serial.begin(115200);
  delay(3000);
  {
    _PL("EspBootStrap Example"); _PL();
#if defined( ARDUINO_ARCH_ESP8266 )
    String id = "ESP8266-" + String(ESP.getChipId(), HEX);
#endif
#if defined( ARDUINO_ARCH_ESP32 )
    String id= "ESP32-" + String((uint32_t)( ESP.getEfuseMac() & 0xFFFFFFFFL ), HEX);
#endif
    _PP("ESP Chip ID: "); _PL(id);
    _PP("Parameter structure size: "); _PL( sizeof(Params) );
    _PL();
  }
#endif

  // **** PARAMETERS COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // To work with Parameters we need to instanciate a Paramters object.
  //  first parameter  is the starting address in the EEPROM memory, {int}
  //  second parameter is a reference to the token, {String}
  //  third parameter  is a pointer to the parameters variable {Params}
  //  fourth parameter is a pointer to the defaults {cahr **}, or NULL if no defaults
  //  fifth parameter is a size of paramters structure + 1 byte for CRC
  Parameters<Params> *p = new Parameters<Params>(0, TOKEN, &eg, &defaults, sizeof(Params) + 1);

  // Define EEPROM_MAX to explicitly set maximum EEPROM capacity for your chip
  // begin() method actaully checks if there is enough space in the EEPROM and return PARAMS_ERR if not
  rc = p->begin();
  _PP(": EspBootStrap initialized. rc = ");_PL(rc);

  // load() methods attempts to load parameters from the EEPROM
  // It checks for a valid CRC and a match on the TOKENs.
  // If either CRC or token do not match, an error is set
  // load() can return OK, indicating that in the end the structure was populated with something,
  // however lastError() may indicate that values were overwritten with defaults due to CRC
  // or Token mismatch, and therefore return a non-zero value. Always check lastError()!
  rc = p->load();
  _PP("Configuration loaded. rc = "); _PL(rc);
  printConfig();

  // If Parameters were loaded successfully, the device will try to
  // connect to the WiFi network specified in the configuration for 30 seconds
  // It will time out after 30 seconds on an assumption that WiFi config is invalid
  // 30 seconds is arbitrary - you can set it for longer of shorter period of time
  if (rc == PARAMS_OK) {
    _PP("Connecting to WiFi for 30 sec:");
    setupWifi(eg.ssid, eg.pwd);
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
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
    _PL("Bootstrapping...");

    // **** BOOTSTRAP COMPONENT OF ESPBOOTSTRAP LIBRARY ****
    // =====================================================
    // ESPBootstrap.run() takes 4 paramters:
    //  Pointer to the descriptions (title, and fields), {char **}
    //  Pointer to the parameter map, {char **}
    //  Number of parameters to display on the web form, {int}
    //  Timeout in milliseconds. (can use helper constants BOOTSTRAP_MINUTE and BOOTSTRAP_SECOND)
    rc = ESPBootstrap.run(PAGE, PARS, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);

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
    delay(1000);
    ESP.restart();
  }

  // **** JSONCONFIG COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // The rest of the parameters could be lifted off a JSON configuration file
  // Example of the JSON file is provided on the github.
  // JSON file rules:
  //  1. No arrays - just a list of key-value pairs
  //  2. All keys and values are strings and should be in quotation marks
  //  3. The order of key-value pairs in the file should be EXACTLY the same as paramters structure, without leading TOKEN
  //  4. Keys and values can contain backslash-ed characters: e.g.; "key\"with a quote" : "\\path\\path2",
  //  5. Each line should end with a comma "," (except the last line)
  //
  // JSONConfig is a static singleton object that does HTTP call, parses the JSON file, and
  // populates respective confuration fields.
  // JSONConfig.parseHttp takes 3 parameters:
  //  a fully quallified URL pointing to a JSON configuation file (including http://) {char *}
  //  a pointer to the parameter map, {char **}
  //  number of paramters to populate minus 1 for token, {int}
  rc = JSONConfig.parseHttp(eg.cfg_url, PARS, NPARS - 1);

  // If successful, the "eg" structure should have a fresh set of paraeters from the JSON file.
  _PP("JSONConfig finished. rc = "); _PL(rc);
  printConfig();
  if (rc == 0) p->save();
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
// Returns true if connection times out
bool waitForWifi(unsigned long aTimeout) {
  _PL("WaitForWifi()");

  unsigned long timeNow = millis();

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    _PP(".");
    if ( millis() - timeNow > aTimeout ) {
      _PL(" WiFi connection timeout");
      return true;
    }
  return false;
  }

  _PL(" WiFi connected");
  _PP("IP address: "); _PL(WiFi.localIP());
  _PP("SSID: "); _PL(WiFi.SSID());
  _PP("mac: "); _PL(WiFi.macAddress());
}
