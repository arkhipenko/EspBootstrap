/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This is exampe 3 without comments and debug messages

*/

#define  EEPROM_MAX 4096
#define CTOKEN  "EBS3"

// ====================================
#define SSID1 "YOUR WIFI SSID HERE"
#define PWD1  "YOUR WIFI PASSWORD HERE"


#include <ParametersEEPROM.h>
#include <EspBootstrapDict.h>
#include <JsonConfigHttp.h>

const String TOKEN(CTOKEN);
const int NPARS = 7;

Dictionary d(NPARS);
const int NPARS_BTS = 3;

void setup(void) {
  int rc;            // Return code of the operations
  bool wifiTimeout;  // Detect a WiFi connection timeout.

  d("Title", "EspBootstrap Alt");
  d("ssid", SSID1);
  d("password", PWD1);
  d("cfg_url", "http://raw.githubusercontent.com/arkhipenko/EspBootstrap/master/examples/EBS_example03_ParametersEEPROM_alt/config.json");

  ParametersEEPROM *p_ptr = new ParametersEEPROM(TOKEN, d, 0, 1024);
  ParametersEEPROM& p = *p_ptr;

  rc = p.begin();
  rc = p.load();

  setupWifi(d["ssid"].c_str(), d["password"].c_str());
  wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);

  if ( !wifiTimeout ) {
    rc = JSONConfig.parse( d["cfg_url"], d);
    if (rc == JSON_OK) {
      p.save();
      d("saved", "ok");
    }
  }

  if ( wifiTimeout || !( rc == JSON_OK || d("saved") ) ) {
    rc = ESPBootstrap.run(d, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);

    if (rc == BOOTSTRAP_OK) {
      p.save();
    }
    delay(5000);
    ESP.restart();
  }

  // Continue with other setup() activities
}

void loop(void) {
}

void setupWifi(const char* ssid, const char* pwd) {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
}

bool waitForWifi(unsigned long aTimeout) {
  unsigned long timeNow = millis();

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    if ( millis() - timeNow > aTimeout ) {
      return true;
    }
  }
  return false;
}
