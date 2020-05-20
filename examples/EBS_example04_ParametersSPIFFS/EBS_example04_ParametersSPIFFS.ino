/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This example illustrates how to use EspBootstrap library with parameters saved on a filesystem.

   1. Compile and upload into the chip
   2. Start Serial monitor
   3. Boot up the chip for the first time
   4. Chip will attempt to load parameters and connect to WiFi
   5. If not successful - chip will create an Access Point names ESPNNN-XXXXXX
      a. Connect your phone/PC to that Access Point. There is no password
      b. Connect your browser to http://10.1.1.1. You have 5 minutes to do steps 7-9 below
      c. Populate the Wifi Settings
      d. Press "Submit" button.
      e. Device will reboot
   6. Device should connect to your WiFi network and read, parse and save the configuration to SPIFFS filesystem

*/

#define CTOKEN  "EBS4"

// ====================================
#define SSID1 "YOUR WIFI SSID HERE"
#define PWD1  "YOUR WIFI PASSWORD HERE"

#define _DEBUG_
//#define _LIBDEBUG_
//#define _TEST_

#include <ParametersSPIFFS.h>
#include <EspBootstrapDict.h>



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

// ==== CODE ==============================

// This methods prints out parameters for inspection
void printConfig() {
  _PL();
  for (int i = 0; i < d.count(); i++) {
    _PP(d(i)); _PP(" : "); _PL(d[i]);
  }
  _PP("Current count = "); _PL(d.count());
  _PP("Current size  = "); _PL(d.size());
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
    _PL("EspBootStrap SPIFFS Dict Example"); _PL();
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
  // Configuration is saved in the JSON file on the SPIFFS filesystem.

  // STEP 1: The default wifi connection is attempted, and device will
  // enter bootstrap mode if wifi connection could not be established

  //  First entry is a Web Form "Title" (element #0)
  d("Title", "EspBootstrap Filesystem");
  //  The rest will be created as fields on the form:
  d("ssid", SSID1);
  d("password", PWD1);
  d("test4", "This is a test");


  // VERY IMPORTANT:
  // Activation of the file system is the responsibility of the programmer
  // You may be using filesystem for other purposes, so the library should
  // not turn it on or off.
  SPIFFS.begin();
  
  // **** PARAMETERS COMPONENT OF ESPBOOTSTRAP LIBRARY ****
  // ======================================================
  // To work with Parameters we need to instanciate a Paramters object.
  //  first parameter is a reference to the token, {String}
  //  second parameter  is a reference to the dictionary object holding parameters {Dictionary}
  //  Since parameters are stored in a json file in the SPIFFS file system, the size of the file
  //  does not matter.
  ParametersSPIFFS *p_ptr = new ParametersSPIFFS(TOKEN, d);
  ParametersSPIFFS& p = *p_ptr;

  rc = p.begin();
  _PP("EspBootStrap ParametersSPIFFS initialized. rc = "); _PL(rc);

  // load() methods attempts to load parameters from the file on SPIFFS filesystem
  // The filename is same as the token. If the token is "EBS4", then the filename will be
  // "/EBS4.json".
  rc = p.load();
  _PP("Configuration loaded. rc = "); _PL(rc);
  printConfig();

  // The device will try to connect to the WiFi network (d["ssid"], d["password"])
  // specified in the configuration, which could be part of the saved parameters or defaults.
  // It will time out after 30 seconds on an assumption that WiFi config is invalid
  // 30 seconds is arbitrary - you can set it for longer of shorter period of time

  _PP("Connecting to WiFi for 30 sec:");
  setupWifi(d["ssid"].c_str(), d["password"].c_str());
  wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);

  // STEP 2:
  // Now we are ready to assess what happened with our attempt to connect to WiFi,
  // load existing parameters, and parse JSON configuration from the config server.
  // The device will need bootstrapping in case of:
  //   1. WiFi timed out  -OR-
  //   2. JSON config parsing ended up in error and a good set of parameters was never saved
  if ( wifiTimeout ) {
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
      rc = p.save();
      _PL("Bootstrapped OK. Rebooting.");
      _PP("Parametes saved. rc = "); _PL(rc);
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
  printConfig();

  // Continue with other setup() activities
  SPIFFS.end();
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
