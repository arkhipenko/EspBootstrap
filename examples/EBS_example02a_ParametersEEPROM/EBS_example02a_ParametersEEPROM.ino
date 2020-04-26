/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This is example #2 without comments and debug messages
*/

#define  EEPROM_MAX 4096
#define CTOKEN  "EBS2a"

#include <ParametersEEPROM.h>
#include <EspBootstrapDict.h>
#include <JsonConfigHttp.h>

const String TOKEN(CTOKEN);
const int NPARS = 7;

Dictionary d(NPARS);
const int NPARS_BTS = 3;


// ==== CODE ==============================

void setup(void) {
  int rc;
  bool wifiTimeout;

  d("Title", "EspBootstrapD");
  d("ssid", "<wifi ssid>");
  d("pwd", "<wifi password>");
  d("cfg_url", "http://raw.githubusercontent.com/arkhipenko/EspBootstrap/master/examples/EBS_example02_ParametersEEPROM/config.json");

  ParametersEEPROM *p_ptr = new ParametersEEPROM(TOKEN, d, 0, 1024);
  ParametersEEPROM& p = *p_ptr;

  rc = p.begin();
  rc = p.load();

  if (rc == PARAMS_OK) {
    setupWifi(d["ssid"].c_str(), d["pwd"].c_str());
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
  }
  if (rc != PARAMS_OK || wifiTimeout) {
    rc = ESPBootstrap.run(d, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);
    if (rc == BOOTSTRAP_OK) {
      p.save();
    }
    delay(5000);
    ESP.restart();
  }

  rc = JSONConfig.parse(d["cfg_url"], d);
  if (rc == 0) p.save();
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
