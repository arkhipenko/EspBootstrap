/*
Copyright (c) 2015-2020, Anatoli Arkhipenko.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _ESPBOOTSTRAPDICT_H_
#define _ESPBOOTSTRAPDICT_H_


#include <Arduino.h>
#include <EspBootstrapBase.h>
#include <Dictionary.h>

class EspBootstrapDict : public EspBootstrapBase {
  public:
    EspBootstrapDict();
    virtual ~EspBootstrapDict();

    int8_t    run(Dictionary &aDict, uint8_t aNum = 0, uint32_t aTimeout = 10 * BOOTSTRAP_MINUTE);
    void      handleRoot ();
    void      handleSubmit ();
    inline void cancel() { iCancelAP = true; } ;
    

  private:
    int8_t            doRun();

    bool              iCancelAP;
    Dictionary*       iDict;

};

static EspBootstrapDict ESPBootstrap;

EspBootstrapDict::EspBootstrapDict () {
}


EspBootstrapDict::~EspBootstrapDict () {
}


void __espbootstrap_handleroot() {
  ESPBootstrap.handleRoot();
}


void __espbootstrap_handlesubmit() {
  ESPBootstrap.handleSubmit();
}


int8_t EspBootstrapDict::run(Dictionary &aDict, uint8_t aNum, uint32_t aTimeout) {
  if (aNum == 0) {
    iNum = aDict.count() - 1;
  }
  else {
    iNum = aNum;
  }
  if (iNum > aDict.count() - 1) iNum = aDict.count() - 1;
  if (iNum == 0) iNum = 1;

  iDict = &aDict;
  iTimeout = aTimeout;

  iCancelAP = false;
  return doRun();
}


int8_t EspBootstrapDict::doRun() {

  String ssid(SSID_PREFIX);
  const IPAddress   APIP   (10, 1, 1, 1);
  const IPAddress   APMASK (255, 255, 255, 0);

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);

  ssid += WiFi.macAddress();
  ssid.replace(":", "");
  ssid.toLowerCase();
//#if defined( ARDUINO_ARCH_ESP8266 )
//  ssid += String(ESP.getChipId(), HEX);
//#endif
#if defined( ARDUINO_ARCH_ESP32 )
//  ssid += String((uint32_t)( ESP.getEfuseMac() & 0xFFFFFFFFL ), HEX);
  WiFi.softAP( ssid.c_str());
  delay(50);
#endif

  WiFi.softAPConfig(APIP, APIP, APMASK);
  delay(50);
  WiFi.softAP( ssid.c_str());
  yield();

  iServer = new WebServer(80);
  if (iServer == NULL) return BOOTSTRAP_ERR;

  iServer->on("/submit.html", __espbootstrap_handlesubmit);
  iServer->onNotFound(__espbootstrap_handleroot);

  iAllDone = false;
  iServer->begin();

  uint32_t timeNow = millis();
  while (!iAllDone && !iCancelAP) {
    iServer->handleClient();
    if ( millis() - timeNow > iTimeout ) {
        iServer->stop();
        iServer->close();
        delete iServer;
        iServer = NULL;
        return BOOTSTRAP_TIMEOUT;
    }
    delay(10);
//    yield();
  }

  iServer->stop();
  iServer->close();
  delete iServer;
  iServer = NULL;
  return (iCancelAP ? BOOTSTRAP_CANCEL: BOOTSTRAP_OK);
}


#define BUFLEN 256
void EspBootstrapDict::handleRoot() {
  char buf[BUFLEN];

  iServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  iServer->send(200, "text/html", "" );
  iServer->sendContent("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/></head><body>");

  Dictionary& d = *iDict;
  snprintf(buf, BUFLEN, "<h2 style=\"color:blue;\">%s</h2><form action=\"/submit.html\">", d[0].c_str() );
  iServer->sendContent(buf);

  for (int i = 1; i <= iNum; i++) {
    String s = d(i);
    s.toUpperCase();
    if ( s.indexOf("PASSWORD") >= 0 || s.indexOf("PWD") >= 0 ) {
      snprintf(buf, BUFLEN, "<label for=\"par%02d\"><b>%s:</b></label><br><input type=\"password\" id=\"par%02d\" name=\"par%02d\" value=\"%s\"><br>", i, d(i).c_str(), i, i, d[i].c_str() );
    }
    else {
      snprintf(buf, BUFLEN, "<label for=\"par%02d\"><b>%s:</b></label><br><input type=\"text\" id=\"par%02d\" name=\"par%02d\" value=\"%s\"><br>", i, d(i).c_str(), i, i, d[i].c_str() );
    }
    iServer->sendContent(buf);
  }
  iServer->sendContent("<br><input type=\"submit\" value=\"Submit\"></form></body></html>");
}


void EspBootstrapDict::handleSubmit() {
  iServer->setContentLength(CONTENT_LENGTH_UNKNOWN);
  iServer->send(200, "text/html", "" );
  iServer->sendContent("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/></head><body><h2 style=\"color:blue;\">Saved</h2>");
  iServer->sendContent("<p>Your changes are saved</p></body></html>");

  Dictionary& d = *iDict;
  for (int i = 0; i < iServer->args() && i < iNum; i++) {
    d( d(i + 1), iServer->arg(i) );
  }
  iAllDone = true;
}



#endif // _ESPBOOTSTRAPDICT_H_