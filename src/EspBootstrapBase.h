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

#ifndef _ESPBOOTSTRAPBASE_H_
#define _ESPBOOTSTRAPBASE_H_


#include <Arduino.h>


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

class EspBootstrapBase {
  public:
    EspBootstrapBase();
    virtual ~EspBootstrapBase();

  protected:
    int8_t            iAllDone;
    WebServer*        iServer;
    uint8_t           iNum;
    uint32_t          iTimeout;
};


EspBootstrapBase::EspBootstrapBase () {
  iAllDone = false;
}


EspBootstrapBase::~EspBootstrapBase () {
  if (iServer) {
    iServer->stop();
    iServer->close();
    delete iServer;
  }
}

#if defined( ARDUINO_ARCH_ESP8266 )
#define   SSID_PREFIX   "ESP8266-"
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#define   SSID_PREFIX   "ESP32-"
#endif


#ifndef SSID_PREFIX
#define   SSID_PREFIX   "BOOTSTRAP-AP"
#endif


#endif // _ESPBOOTSTRAPBASE_H_