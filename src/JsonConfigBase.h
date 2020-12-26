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

#ifndef _JSONCONFIGBASE_H_
#define _JSONCONFIGBASE_H_


#include <Arduino.h>


#define JSON_OK         0
#define JSON_ERR      (-1)
#define JSON_COMMA    (-20)
#define JSON_COLON    (-21)
#define JSON_QUOTE    (-22)
#define JSON_BCKSL    (-23)
#define JSON_MEM      (-24)
#define JSON_FMT      (-25)
#define JSON_EOF      (-99)

class JsonConfigBase {
  public:
    JsonConfigBase();
    virtual ~JsonConfigBase();
    
  protected:
    virtual int8_t  _doParse(size_t aLen, uint16_t aNum);
    virtual int16_t _nextChar() { return JSON_EOF; };
    virtual int8_t  _storeKeyValue(const char* aKey, const char* aValue) { return JSON_MEM; };
};

JsonConfigBase::JsonConfigBase() {}
JsonConfigBase::~JsonConfigBase() {}

int8_t JsonConfigBase::_doParse(size_t aLen, uint16_t aNum) {

  bool      insideQoute = false;
  bool      nextVerbatim = false;
  bool      isValue = false;
  bool      isComment = false;
  int       p = 0;
  int8_t    rc;
  String    currentKey = String();
  String    currentValue = String();

  for (int i = 0; i < aLen; i++) {
    char c;
    int16_t nrc = _nextChar();
    if ( nrc < 0 ) break; //EOF
    c = (char) nrc;
#ifdef _LIBDEBUG_
Serial.print(c); Serial.print("("); Serial.print((int)c); Serial.print(")");
#endif
    if ( isComment ) {
      if ( c == '\n' ) {
        isComment = false;
        isValue = false;
      }
      continue;
    }
    if (nextVerbatim) {
      nextVerbatim = false;
    }
    else {
      // process all special cases: '\', '"', ':', and ','
      if (c == '\\' ) {
        nextVerbatim = true;
        continue;
      }
      if ( c == '#' ) {
        if ( !insideQoute ) {
          isComment = true;
          continue;
        }
      }
      if ( c == '\"' ) {
        if (!insideQoute) {
          insideQoute = true;
          continue;
        }
        else {
          insideQoute = false;
          if (isValue) {
            rc = _storeKeyValue( currentKey.c_str(), currentValue.c_str() );
            if (rc) return JSON_MEM;  // if error - exit with an error code
            currentValue = String();
            currentKey = String();
            p++;
            if (aNum > 0 && p >= aNum) break;
          }
          continue;
        }
      }
      if (c == '\n') {
        if ( insideQoute ) {
          return JSON_QUOTE;
        }
        if ( nextVerbatim ) {
          return JSON_BCKSL;
        }
        isValue = false;  // missing comma, but let's forgive that
        continue;
      }
      if (!insideQoute) {
        if (c == ':') {
          if ( isValue ) {
            return JSON_COMMA; //missing comma probably
          }
          isValue = true;
          continue;
        }
        if (c == ',') {
          if ( !isValue ) {
            return JSON_COLON; //missing colon probably
          }
          isValue = false;
          continue;
        }
        if ( c == '{' || c == '}' || c == ' ' || c == '\t' ) continue;
        return JSON_FMT;
      }
    }
    if (insideQoute) {
      if (isValue) currentValue.concat(c);
      else currentKey.concat(c);
    }
  }
  if (insideQoute || nextVerbatim || (aNum > 0 && p < aNum )) return JSON_EOF;
#ifdef _LIBDEBUG_
    Serial.printf("JsonConfigBase::_doParse: JSON_OK\n");
#endif
  return JSON_OK;
}



#endif // _JSONCONFIGBASE_H_