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

#ifndef _JSONCONFIGHTTPMAP_H_
#define _JSONCONFIGHTTPMAP_H_


#include <JsonConfigBase.h>

#if defined( ARDUINO_ARCH_ESP8266 )
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#include <WiFiClient.h>
#include <HTTPClient.h>
#endif


#define JSON_HTTPERR  (-97)
#define JSON_NOWIFI   (-98)


class JsonConfigHttpMap : public JsonConfigBase {
  public:
    JsonConfigHttpMap();
    virtual ~JsonConfigHttpMap();
    
    int8_t   parse(const String aUrl, char** aMap, int aNum);
    int8_t   parse(const String aHost, uint16_t aPort, String aUrl, char** aMap, int aNum);
        
  protected:
    virtual int16_t _nextChar();
    virtual int8_t  _storeKeyValue(const char* aKey, const char* aValue);
    virtual int8_t  _doParse(size_t aLen, uint16_t aNum) { return JsonConfigBase::_doParse(aLen, aNum); };
    
  private:
    int8_t          parseCommon(int aHttpResult, int aNum);

    char**          iMap;
    HTTPClient      iHttp;
    String          iPayload;
    size_t          iIndex;
    size_t          iParamIndex;
};

#ifndef _JSONCONFIG_NOSTATIC
static JsonConfigHttpMap JSONConfig;
#endif 

JsonConfigHttpMap::JsonConfigHttpMap() {}
JsonConfigHttpMap::~JsonConfigHttpMap() {}


int8_t JsonConfigHttpMap::parse(const String aHost, uint16_t aPort, const String aUrl, char** aMap, int aNum) {
    int8_t rc;
    WiFiClient      client;
    
    if (WiFi.status() != WL_CONNECTED) return JSON_NOWIFI;
#ifdef _LIBDEBUG_
    Serial.printf("JsonConfig: Connecting to: %s\n", aUrl.c_str());
#endif
    iMap = aMap;
    rc = parseCommon( iHttp.begin(client, aHost, aPort, aUrl), aNum );
    iHttp.end();
    return rc;
}



int8_t JsonConfigHttpMap::parse(const String aUrl, char** aMap, int aNum) {
    int8_t rc;
    WiFiClient      client;

    if (WiFi.status() != WL_CONNECTED) return JSON_NOWIFI;
#ifdef _LIBDEBUG_
    Serial.printf("JsonConfig: Connecting to: %s\n", aUrl.c_str());
#endif
    iMap = aMap;
    rc = parseCommon( iHttp.begin(client, aUrl), aNum );
    iHttp.end();
#ifdef _LIBDEBUG_
    Serial.printf("JsonConfigHttpMap::parse rc %d\n", rc );
#endif 
    return rc;
}


int8_t JsonConfigHttpMap::parseCommon(int aHttpResult, int aNum) {
    int8_t rc;
  
    if ( aHttpResult ) {
        int httpCode = iHttp.GET();
#ifdef _LIBDEBUG_
            Serial.printf("JsonConfig: Connected httpCode= %d\n", httpCode );
#endif
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                iPayload = iHttp.getString();
                iIndex = 0;
                iParamIndex = 0;
                rc = _doParse(iPayload.length(), aNum);
#ifdef _LIBDEBUG_
            Serial.printf("JsonConfigHttpMap::parseCommon rc %d\n", rc );
#endif                
                return rc;
            }
        }
        else {
            return httpCode;
        }
        return JSON_ERR;  // should never get here anyway - but stupid compiler complains. 
    }
    else {
        return JSON_HTTPERR;
    }
    return JSON_ERR;  // should never get here anyway - but stupid compiler complains. 
}


int16_t JsonConfigHttpMap::_nextChar() {
    if (iIndex < iPayload.length() ) {
        return (int16_t) iPayload[iIndex++];
    }
    else {
        return JSON_EOF;
    }
}


int8_t  JsonConfigHttpMap::_storeKeyValue(const char* aKey, const char* aValue){
#ifdef _LIBDEBUG_
    Serial.printf("JsonConfigHttpMap::_storeKeyValue: %s:%s\n", aKey, aValue );
//    Serial.printf("iMap base address: %u, iMap[iParamIndex] address: %u\n", (uint32_t)iMap, (uint32_t)iMap[iParamIndex]);
#endif

    strcpy(iMap[iParamIndex++], aValue);
//    memcpy(iMap[iParamIndex++], aValue, strlen(aValue)+1);
    return JSON_OK;
}

#endif // _JSONCONFIGHTTPMAP_H_