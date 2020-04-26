/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This is exampe 4 without comments and debug messages

*/

#define CTOKEN  "EBS4"

// ====================================
#define SSID1 "YOUR WIFI SSID HERE"
#define PWD1  "YOUR WIFI PASSWORD HERE"

#include <ParametersSPIFFS.h>
#include <EspBootstrapDict.h>

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
  d("test4", "This is a test");

  SPIFFS.begin();

  ParametersSPIFFS *p_ptr = new ParametersSPIFFS(TOKEN, d);
  ParametersSPIFFS& p = *p_ptr;

  rc = p.begin();
  rc = p.load();

  setupWifi(d["ssid"].c_str(), d["password"].c_str());
  wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);

  if ( wifiTimeout ) {
    rc = ESPBootstrap.run(d, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);

    if (rc == BOOTSTRAP_OK) {
      rc = p.save();
    }
    delay(5000);
    ESP.restart();
  }

  SPIFFS.end();
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
    delay(1000);
    if ( millis() - timeNow > aTimeout ) {
      return true;
    }
  }
  return false;
}
