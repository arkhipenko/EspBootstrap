#ifndef _ESPBOOTSTRAP_H_
#define _ESPBOOTSTRAP_H_


#include <Arduino.h>
#include <Parameters.h>

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


#define BOOTSTRAP_OK        0
#define BOOTSTRAP_ERR      (-1)
#define BOOTSTRAP_TIMEOUT (-99)

#define BOOTSTRAP_SECOND  1000L
#define BOOTSTRAP_MINUTE  60000L

class EspBootstrap {
  public:
    EspBootstrap();
    ~EspBootstrap();

    int8_t     run(uint8_t aNum, const char** aTitles, char** aMap, uint32_t aTimeout = 10 * BOOTSTRAP_MINUTE);
    void       handleRoot ();
    void       handleSubmit ();


  private:
    
    int8_t            iAllDone; 
    WebServer*        iServer;
    uint8_t           iNum;
    const char**      iTitles;
    char**            iMap;
    uint32_t          iTimeout;
};

static EspBootstrap ESPBootstrap;

EspBootstrap::EspBootstrap () {
  iAllDone = false; 
}


EspBootstrap::~EspBootstrap () {
  if (iServer) {
    iServer->stop();
    iServer->close();
    delete iServer;
  }
}



void __espbootstrap_handleroot() {
  ESPBootstrap.handleRoot();
}


void __espbootstrap_handlesubmit() {
  ESPBootstrap.handleSubmit();
}



  
#define   SSID_PREFIX   "BOOTSTRAP-AP"

#if defined( ARDUINO_ARCH_ESP8266 )
#define   SSID_PREFIX   "ESP8266-"
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#define   SSID_PREFIX   "ESP32-"
#endif

int8_t EspBootstrap::run(uint8_t aNum, const char** aTitles, char** aMap, uint32_t aTimeout) {

  iNum = aNum;
  iTitles = aTitles;
  iMap = aMap;
  iTimeout = aTimeout;
  
  String ssid(SSID_PREFIX);
  const IPAddress   APIP   (10, 1, 1, 1);
  const IPAddress   APMASK (255, 255, 255, 0);

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);

#if defined( ARDUINO_ARCH_ESP8266 )
  ssid += String(ESP.getChipId(), HEX);
#endif
#if defined( ARDUINO_ARCH_ESP32 )
  ssid += String((uint32_t)( ESP.getEfuseMac() & 0xFFFFFFFFL ), HEX);
#endif
  WiFi.softAP( ssid.c_str());
  WiFi.softAPConfig(APIP, APIP, APMASK);
  yield();

  iServer = new WebServer(80);
  if (iServer == NULL) return BOOTSTRAP_ERR;

  iServer->on("/submit.html", __espbootstrap_handlesubmit);
  iServer->onNotFound(__espbootstrap_handleroot);
  
  iAllDone = false;
  iServer->begin();
  
  uint32_t timeNow = millis();
  while (!iAllDone) {
    iServer->handleClient();
    if ( millis() - timeNow > iTimeout ) return BOOTSTRAP_TIMEOUT;
  }
  
  return BOOTSTRAP_OK;
  iServer->stop();
  iServer->close();
  delete iServer;
  iServer = NULL;
}


#define BUFLEN 256
void EspBootstrap::handleRoot() {
  char buf[BUFLEN];

  iServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  iServer->send(200, "text/html", "" );
  iServer->sendContent("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/></head><body>");

  snprintf(buf, BUFLEN, "<h2 style=\"color:blue;\">%s</h2><form action=\"/submit.html\">", iTitles[0] );
  iServer->sendContent(buf);

  for (int i = 1; i <= iNum; i++) {
    snprintf(buf, BUFLEN, "<label for=\"par%02d\"><b>%s:</b></label><br><input type=\"text\" id=\"par%02d\" name=\"par%02d\" value=\"%s\"><br>", i,iTitles[i], i, i, iMap[i-1] );
    iServer->sendContent(buf);
  }
  iServer->sendContent("<br><input type=\"submit\" value=\"Submit\"></form></body></html>");
}


void EspBootstrap::handleSubmit() {
  iServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  iServer->send(200, "text/html", "" );
  iServer->sendContent("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/></head><body><h2 style=\"color:blue;\">Saved</h2>");
  iServer->sendContent("<p>Your changes are saved</p></body></html>");

  for (int i = 0; i < iServer->args() && i < iNum; i++) {
    strcpy( iMap[i], iServer->arg(i).c_str() );
  }
  iAllDone = true;
}



#endif // _ESPBOOTSTRAP_H_