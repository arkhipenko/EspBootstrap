/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This example illustrates how litte code is actually required 
   to bootstrap a device using ESPBootstrap libraries. 
   This is example #2 without comments. 

*/

#define _USE_DICTIONARY_
#define  EEPROM_MAX 4096
#define CTOKEN  "EBS1"

#include <Parameters.h>
#include <EspBootstrap.h>
#include <JsonConfig.h>

const String TOKEN(CTOKEN);
const int NPARS = 7;
const int NPARS_BTS = 3;

Dictionary d(NPARS);


void setup(void) {
  bool wifiTimeout;
  int rc;

  d("Title", "EspBootstrapD");
  d("ssid", "<wifi ssid>");
  d("pwd", "<wifi password>");
  d("cfg_url", "http://ota.home.lan/esp/config/ebs02config.json");

  Parameters *p = new Parameters(0, TOKEN, d, 1024);
  if (p->begin() != PARAMS_OK) for(;;) ; // something is wrong with EEPROM!!
  
  if ( (rc = p->load()) == PARAMS_OK) {
    setupWifi( d["ssid"].c_str(), d["pwd"].c_str() );
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
  }

  if (rc != PARAMS_OK || wifiTimeout) {
    if (ESPBootstrap.run(d) == BOOTSTRAP_OK) p->save();
    delay(1000);
    ESP.restart();
  }
  if (JSONConfig.parseHttp(d["cfg_url"], d) == 0) p->save();
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
    if ( millis() - timeNow > aTimeout ) return true;
  return false;
  }
}
