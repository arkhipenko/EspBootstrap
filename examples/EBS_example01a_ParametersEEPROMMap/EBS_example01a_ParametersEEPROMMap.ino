/*
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

   This is example #1 without comments or debug messages
*/

#include <ParametersEEPROMMap.h>  // Parameters stored in EEPROM 
#include <EspBootstrapMap.h>      // ESP Bootstrap for memory structure
#include <JsonConfigHttpMap.h>    // Json parsing from HTTP host into memory structure

#define CTOKEN  "EBS1A"
const String TOKEN(CTOKEN);
const int NPARS = 7;

typedef struct {
  char token[6];
  char ssid[32];
  char pwd[32];
  char cfg_url[128];
  char ota_host[32];
  char ota_port[6];
  char ota_url[32];
} Params;
Params eg;

Params defaults = { CTOKEN,
                    "<wifi ssid>",
                    "<wifi password>",
                    "http://raw.githubusercontent.com/arkhipenko/EspBootstrap/master/examples/EBS_example01_ParametersEEPROMMap/config.json",
                    "<ota.server.com>",
                    "1234",
                    "/esp/ota.php"
                  };

char* PARS[] = { eg.ssid,
                 eg.pwd,
                 eg.cfg_url,
                 eg.ota_host,
                 eg.ota_port,
                 eg.ota_url
               };

const int NPARS_BTS = 3;
const char* PAGE[] = { "EspBootstrap",
                       "WiFi SSID",
                       "WiFi PWD",
                       "Config URL",
                     };

void setup(void) {
  bool wifiTimeout;
  int rc;

  ParametersEEPROMMap *p_ptr = new ParametersEEPROMMap(TOKEN, &eg, &defaults, 0, sizeof(Params));
  ParametersEEPROMMap& p = *p_ptr;

  rc = p.begin();
  rc = p.load();

  if (rc == PARAMS_OK) {
    setupWifi(eg.ssid, eg.pwd);
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
  }

  if (rc != PARAMS_OK || wifiTimeout) {
    rc = ESPBootstrap.run(PAGE, PARS, NPARS_BTS, 5 * BOOTSTRAP_MINUTE);
    if (rc == BOOTSTRAP_OK) {
      p.save();
    }
    delay(1000);
    ESP.restart();
  }
  rc = JSONConfig.parse(eg.cfg_url, PARS, NPARS - 1);
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
    delay(1000);
    if ( millis() - timeNow > aTimeout ) {
      return true;
    }
  }
  return false;
}
