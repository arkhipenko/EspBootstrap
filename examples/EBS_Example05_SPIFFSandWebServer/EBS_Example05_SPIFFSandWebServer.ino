/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This example illustrates how to to read/write parameters to the filesystem and
   add/delete entries to the configuration dictionary object.

   For this example to run you need to upload the "data" folder into the device's SPIFFS
   filesystem using ESP8266 or ESP32 Sketch Data Upload tools
   https://github.com/esp8266/arduino-esp8266fs-plugin.git
   https://github.com/me-no-dev/arduino-esp32fs-plugin.git

*/

//#define _DICT_CRC64_
#include <ParametersSPIFFS.h>
#include <EspBootstrapDict.h>


// Includes for running the webserver
// SPIFFS browsing example.
#if defined( ARDUINO_ARCH_ESP8266 )
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define WebServer ESP8266WebServer
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#include <WiFi.h>
#include <WebServer.h>
#define WebServer WebServer
#endif

#include <WiFiClient.h>

// The next header file needs to be created for your network
// I place it into "libraries/MySettings" folder.
// It should contain SSID1 and PWD1 defines
// e.g.,
//  #define SSID1 "your ssid"
//  #define PWD1  "your password"
#include "home_wifi_multi.h"

FS* filesystem = &SPIFFS;
WebServer server(80);
File fsUploadFile;

// Prameters token
#define CTOKEN  "EBS5"

// Enable short debug printouts
#define _DEBUG_
//#define _TEST_

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif


const String TOKEN(CTOKEN);

// Create dictionary object and parameters object
// to be stored on the filesystem
Dictionary d;
ParametersSPIFFS p(TOKEN, d);


// Utility method to print contents of a dictionary
void printD(Dictionary& a) {
  _PL("\nDictionary content:");
  for (int i = 0; i < a.count(); i++) {
    _PP(a(i)); _PP(" : "); _PL(a[i]);
  }
  _PP("Current count = "); _PL(a.count());
  _PP("Current size  = "); _PL(a.size());
  _PL();
}


void setup(void) {
  bool wifiTimeout;
  int rc;
  String id;

#ifdef _DEBUG_
  Serial.begin(115200);
  delay(500);
  {
    _PL("\n\nEspBootStrap SPIFFS Example"); _PL();
#if defined( ARDUINO_ARCH_ESP8266 )
    id = "ESP8266-" + String(ESP.getChipId(), HEX);
#endif
#if defined( ARDUINO_ARCH_ESP32 )
    id = "ESP32-" + String((uint32_t)( ESP.getEfuseMac() & 0xFFFFFFFFL ), HEX);
#endif
    _PP("ESP Chip ID: "); _PL(id);
    _PL();
  }
#endif

  //  The sketch is responsible for activating SPIFFS
  SPIFFS.begin();

  // Populate the dictionary with test values
  d("This", "is test");
  d("Title", "ESP Test 04");
  d("ssid", SSID1);
  d("pwd", PWD1);
  d("cfg_url", String("/") + TOKEN + ".json");
  printD(d);

  // Start the parameters object
  if ( (rc=p.begin()) != PARAMS_OK) {
    _PP("Parameters begin returned an error: "); _PL(rc);
    for (;;) ; // something is wrong with EEPROM!!
  }

  // Load values from the file on SPIFFS system
  // and then try to connect to wifi
  if ( (rc = p.load()) == PARAMS_OK) {
    setupWifi(d["ssid"].c_str(), d["pwd"].c_str());
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
  }

  // Bootstrap the device if paramters load was not successful
  // or if connection to wifi failed.
  if (rc != PARAMS_OK || wifiTimeout) {
    _PL("Device requires bootstrapping...");
    _PP("Connect to WiFi AP: "); _PL(id);
    _PL("and navigate to http://10.1.1.1");
    if ( ESPBootstrap.run(d) == BOOTSTRAP_OK ) p.save();
    delay(1000);
    ESP.restart();
  }

  // Try saving parameters into JSON file again
  // testing overwrite
  rc = p.save();
  _PP("Param File Save finished. rc = "); _PL(rc);

  // Delete all entries from the dictionary
  _PL("Deleting all entries in the dictionary");
  while (d.count()) {
    d.remove(d(0));
  }
  printD(d);

  // Load all entries from the file back into the dictionaries
  _PL("Loading all entries back");
  rc = p.load();
  
  // and add one more line to test dictionary still works.
  d("new line", "new value");
  printD(d);

  // Set up webserver to allow browing of the file system
  setupServer();
}

void loop(void) {
  server.handleClient();
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
  _PL();
  _PP("Navigate to http://"); _PP(WiFi.localIP()); _PL("/edit");
  return false;
}
