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

#ifndef _JSONCONFIGSPIFFSMAP_H_
#define _JSONCONFIGSPIFFSMAP_H_


#include <JsonConfigBase.h>

#if defined( ARDUINO_ARCH_ESP8266 )
#include <FS.h>
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#include <FS.h>
#include <SPIFFS.h>
#endif


#define JSON_FILERR  (-95)
#define JSON_FILENE  (-96)


class JsonConfigSPIFFSMap : public JsonConfigBase {
  public:
    JsonConfigSPIFFSMap();
    virtual ~JsonConfigSPIFFSMap();
    
    int8_t   parse(const String aUrl, char** aMap, int aNum);
    
  protected:
    virtual int8_t  _storeKeyValue(const char* aKey, const char* aValue);
    virtual int8_t  _doParse(Stream& aJson, uint16_t aNum) { return JsonConfigBase::_doParse(aJson, aNum); };
        
  private:
    char**          iMap;
    File            iF;
    size_t          iParamIndex;
};

#ifndef _JSONCONFIG_NOSTATIC
static JsonConfigSPIFFSMap JSONConfig;
#endif

JsonConfigSPIFFSMap::JsonConfigSPIFFSMap() {}
JsonConfigSPIFFSMap::~JsonConfigSPIFFSMap() {}

int8_t JsonConfigSPIFFSMap::parse(const String aUrl, char** aMap, int aNum) {
  int8_t rc; 
  
  if ( !SPIFFS.exists(aUrl) ) { // || !SPIFFS.isFile(aUrl) ) {
    return JSON_FILENE;
  }

  iF = SPIFFS.open(aUrl, "r");
  if ( !iF ) {
    return JSON_FILERR;
  }

  iMap = aMap;
  iParamIndex = 0;
  rc = _doParse ( iF, aNum );
  
  iF.close();
  return rc;
}


// char    JsonConfigSPIFFSMap::_nextChar() {
    // return (int16_t) iF.read();
// }


int8_t  JsonConfigSPIFFSMap::_storeKeyValue(const char* aKey, const char* aValue){
    strcpy(iMap[iParamIndex++], aValue);
    return JSON_OK;
}

#endif // _JSONCONFIGSPIFFSMAP_H_