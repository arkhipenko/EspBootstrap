#ifndef _JSONCONFIG_H_
#define _JSONCONFIG_H_


#include <Arduino.h>
#if defined( ARDUINO_ARCH_ESP8266 )
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#endif

#if defined( ARDUINO_ARCH_ESP32 )
#include <WiFiClient.h>
#include <HTTPClient.h>
#endif

#define JSON_OK         0
#define JSON_ERR      (-1)
#define JSON_COMMA    (-2)
#define JSON_COLON    (-3)
#define JSON_QUOTE    (-4)
#define JSON_BCKSL    (-5)
#define JSON_HTTPERR  (-97)
#define JSON_NOWIFI   (-98)
#define JSON_EOF      (-99)

class JsonConfig {
  public:
    JsonConfig();
    ~JsonConfig();

    int8_t   parseHttp(const String aUrl, char** aMap, int aNum);

  private:
    char**        iMap;
    int           iNum;
};

static JsonConfig JSONConfig;

JsonConfig::JsonConfig() {}
JsonConfig::~JsonConfig() {}

int8_t JsonConfig::parseHttp(const String aUrl, char** aMap, int aNum) {
  WiFiClient client;
  HTTPClient http;
  String payload; 
  
  if (WiFi.status() != WL_CONNECTED) return JSON_NOWIFI;
  if (http.begin(client, aUrl)) { 
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = http.getString();

        bool insideQoute = false;
        bool nextVerbatim = false;
        bool isValue = false;
        const char* c = payload.c_str();
        int len = payload.length();
        int p = 0;
        String currentValue;

        for (int i = 0; i < len; i++, c++) {
          char toAdd = *c;
          if (nextVerbatim) {
            nextVerbatim = false;
          }
          else {
            // process all special cases: '\', '"', ':', and ','
            if (*c == '\\' ) {
              nextVerbatim = true;
              continue;
            }
            if (*c == '\"') {
              if (!insideQoute) {
                insideQoute = true;
                continue;
              }
              else {
                insideQoute = false;
                if (isValue) {
                  strcpy(aMap[p++], currentValue.c_str());
                  if (p >= aNum) break;
                }
                currentValue = String();
              }
            }
            if (*c == '\n') {
              if ( insideQoute ) return JSON_QUOTE;
              if ( nextVerbatim ) return JSON_BCKSL;
            }
            if (!insideQoute) {
              if (*c == ':') {
                if ( isValue ) return JSON_COMMA; //missing comma probably
                isValue = true;
                continue;
              }
              if (*c == ',') {
                if ( !isValue ) return JSON_COLON; //missing colon probably
                isValue = false;
                continue;
              }
            }
          }
          if (insideQoute) currentValue.concat(*c);
        }
        if (insideQoute || nextVerbatim || p < aNum) return JSON_EOF;
        return JSON_OK;
      }
    }
    else {
      return httpCode;
    }
    http.end();
  } 
  else {
    return JSON_HTTPERR;
  }
}

#endif // _JSONCONFIG_H_