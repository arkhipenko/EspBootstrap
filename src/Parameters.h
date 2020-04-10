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

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_


#include <Arduino.h>
#include <EEPROM.h>

#ifndef EEPROM_MAX
#if defined( ARDUINO_ARCH_AVR )
#define EEPROM_MAX  512
#endif

#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
#define EEPROM_MAX  4096
#endif

#endif // #ifndef EEPROM_MAX


// Error codes:
#define PARAMS_OK   0
#define PARAMS_ERR  (-1)
#define PARAMS_LEN  (-2)
#define PARAMS_CRC  (-3)
#define PARAMS_TOK  (-4)
#define PARAMS_MEM  (-98)
#define PARAMS_ACT  (-99)


// ==== IMPLEMENTATION WITH DICTIOANRY OBJECT =============================================
#ifdef _USE_DICTIONARY_
#include <Dictionary.h>

class Parameters {
  public:
    Parameters(uint16_t aAddress, const String& aToken, Dictionary& aDict, uint16_t aSize );
    ~Parameters();

    inline int8_t   lastError() {
      return iRc;
    }
    int8_t          load();
    int8_t          save();
    void            clear();

  private:
    uint8_t         checksum ();

    int8_t          iRc;
    int8_t          iActive;
    Dictionary&     iDict;
    const String&   iToken;
    uint16_t        iAddress;
    uint8_t*        iData;
    uint16_t        iSize;
};

Parameters::Parameters(uint16_t aAddress, const String& aToken, Dictionary& aDict, uint16_t aSize ) : iToken(aToken), iDict(aDict)  {
  iRc = PARAMS_OK;
  iActive = false;
  iAddress = aAddress;
  iSize = aSize;
  iData = NULL;
  uint16_t maxLen = aToken.length() + aDict.size() + 4; // 4: 1 null for token, 1 crc8, 2 bytes for count
  if ( aSize < EEPROM_MAX && maxLen <= aSize) {
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.begin(iSize);
#endif
    iActive = true;
  }
  else {
    iRc = PARAMS_LEN;
  }
}

Parameters::~Parameters() {
  if (iActive) {
    save();
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.end();
#endif
    iActive = false;
  }
}

int8_t Parameters::load() {
  uint16_t iTl = iToken.length();

  if (!iActive) {
    iRc = PARAMS_ACT;
    return PARAMS_ACT;
  }

  iData = (uint8_t * ) malloc(iSize);
  if (iData == NULL) {
    iRc = PARAMS_MEM;
    return PARAMS_MEM;
  }
  uint8_t* p = iData;

  for (uint16_t i = 0; i < iSize - 1; i++, p++) {
    *p = EEPROM.read(iAddress + i);
  }
  uint8_t crc = EEPROM.read( iAddress + iSize - 1);

  // Check CRC
  if (crc != checksum () ) {
//    loadDefaults();
    free(iData);
    iData = NULL;
    iRc = PARAMS_CRC;
    return PARAMS_CRC;
  }

  // Check Token
  if ( strncmp( (const char *) iToken.c_str(), (const char *) iData, iSize ) != 0 ) {
//    loadDefaults();
    free(iData);
    iData = NULL;
    iRc = PARAMS_TOK;
    return PARAMS_TOK;
  }

  // Populate the dictionary
  p = iData + (iTl + 1);

  uint16_t cnt = *p | ((((uint16_t)*(p + 1)) << 8) & 0xff00);
  p += 2;

  for (uint16_t i = 0; i < cnt; i++) {
    String k = String((char* )p);
    p += (k.length() + 1);
    String v = String((char* )p);
    p += (v.length() + 1);
    iDict(k, v);
  }

  free(iData);
  iData = NULL;
  iRc = PARAMS_OK;
  return PARAMS_OK;
}


int8_t Parameters::save() {
  if (!iActive) {
    iRc = PARAMS_ACT;
    return PARAMS_ACT;
  }
  uint16_t iTl = iToken.length();
  uint16_t iDs = iDict.size();
  uint16_t iDc = iDict.count();
  uint16_t maxLen = iTl + iDs + 4;

  if ( maxLen >= iSize ) {
    iRc = PARAMS_LEN;
    return PARAMS_LEN;
  }
  iData = (uint8_t * ) malloc(iSize);
  if (iData == NULL) {
    iRc = PARAMS_MEM;
    return PARAMS_MEM;
  }
  clear();
  uint8_t* p = iData;

  strcpy((char*)p, iToken.c_str());
  p += (iTl + 1);

  *p++ = iDc & 0xff;
  *p++ = (iDc >> 8) & 0xff;

  for (uint16_t i = 0; i < iDc; i++) {
    strcpy((char*)p, iDict(i).c_str());
    p += (iDict(i).length() + 1);
    strcpy((char*)p, iDict[i].c_str());
    p += (iDict[i].length() + 1);
  }

  p = iData;

  for (uint16_t i = 0; i < iSize-1; i++, p++) {
#if defined( ARDUINO_ARCH_AVR )
    EEPROM.update(iAddress + i, *p);
#else
    EEPROM.write(iAddress + i, *p);
#endif
  }

#if defined( ARDUINO_ARCH_AVR )
  EEPROM.update( iAddress + iSize - 1, checksum () );
#else
  EEPROM.write( iAddress + iSize - 1, checksum () );
#endif
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
  EEPROM.commit();
#endif

  free(iData);
  iData = NULL;

  iRc = PARAMS_OK;
  return PARAMS_OK;
}


//void Parameters::loadDefaults () {
//}


void Parameters::clear () {
  if (iData) {
    memset((void *) iData, 0, iSize - 1);
  }

}


#define CRCMASK 0x1d
uint8_t Parameters::checksum () {
  uint8_t crc = 0;
  uint8_t *ptr = (uint8_t *) iData;


  for (int j = 0; j < iSize - 1; j++, ptr++) {
    crc ^= *ptr;
    for (int i = 0; i < 8; i++) {
      if ( crc & 0x80 )
        crc = (uint8_t)((crc << 1) ^ CRCMASK);
      else
        crc <<= 1;
    }
  }
  return crc;
}



// ==== IMPLEMENTATION WITH TEMPLATE AND MEMROY MAPPING =============================================
#else

template<typename T>
class Parameters {
  public:
    Parameters(uint16_t address, const String& token, T* ptr, T* deflt = NULL, uint16_t maxlength = EEPROM_MAX );
    ~Parameters();

    inline int8_t   lastError() {
      return iRc;
    }
    int8_t          load();
    int8_t          save();
    void            loadDefaults();
    void            clear();

  private:
    uint8_t         checksum ();

    int8_t          iRc;
    int8_t          iActive;
    T*              iData;
    T*              iDefault;
    const String&   iToken;
    uint16_t        iAddress;
    uint16_t        iLen;
    uint16_t        iMaxLen;

};

template<typename T>
Parameters<T>::Parameters(uint16_t address, const String& token, T* ptr, T* deflt, uint16_t maxlength ) : iToken(token)  {
  iRc = PARAMS_OK;
  iActive = false;
  iAddress = address;
  //  iToken = (String&) token;
  iData = ptr;
  iDefault = deflt;
  iLen = sizeof(T);
  iMaxLen = maxlength;
  if ( iMaxLen < 4 ) iMaxLen = 4;
  if ( iMaxLen >= EEPROM_MAX - 1 ) iMaxLen = EEPROM_MAX - 1;
  if ( iLen + 1 <= iMaxLen ) {
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.begin(iLen + 1);
#endif
    iActive = true;
  }
  else {
    iRc = PARAMS_LEN;
  }
}

template<typename T>
Parameters<T>::~Parameters() {
  if (iActive) {
    save();
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.end();
#endif
    iActive = false;
  }
}

template<typename T>
int8_t Parameters<T>::load() {
  uint8_t *ptr = (uint8_t *) iData;

  if (!iActive) {
    iRc = PARAMS_ACT;
    return PARAMS_ACT;
  }

  for (uint16_t i = 0; i < iLen; i++, ptr++) {
    *ptr = EEPROM.read(iAddress + i);
  }
  uint8_t crc = EEPROM.read( iAddress + iLen);

  if (crc != checksum () ) {
    loadDefaults();
    iRc = PARAMS_CRC;
    return PARAMS_OK;
  }

  if ( strncmp( (const char *) iToken.c_str(), (const char *) iData, iMaxLen ) != 0 ) {
    loadDefaults();
    iRc = PARAMS_TOK;
    return PARAMS_OK;
  }
  iRc = PARAMS_OK;
  return PARAMS_OK;
}


template<typename T>
int8_t Parameters<T>::save() {
  uint8_t *ptr = (uint8_t *) iData;

  if (!iActive) {
    iRc = PARAMS_ACT;
    return PARAMS_ACT;
  }

  if ( sizeof(T) >= iMaxLen ) {
    iRc = PARAMS_LEN;
    return PARAMS_LEN;
  }
  for (uint16_t i = 0; i < iLen; i++, ptr++) {
#if defined( ARDUINO_ARCH_AVR )
    EEPROM.update(iAddress + i, *ptr);
#else
    EEPROM.write(iAddress + i, *ptr);
#endif
  }
#if defined( ARDUINO_ARCH_AVR )
  EEPROM.update( iAddress + iLen, checksum () );
#else
  EEPROM.write( iAddress + iLen, checksum () );
#endif
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
  EEPROM.commit();
#endif

  iRc = PARAMS_OK;
  return PARAMS_OK;
}



template<typename T>
void Parameters<T>::loadDefaults () {
  if ( iDefault ) {
    memcpy((void *) iData, (void *) iDefault, iLen);
  }
  else {
    clear();
  }
  strcpy((char *) iData, iToken.c_str());
}


template<typename T>
void Parameters<T>::clear () {
  memset((void *) iData, 0, iLen);
}


#define CRCMASK 0x1d
template<typename T>
uint8_t Parameters<T>::checksum () {
  uint8_t crc = 0;
  uint8_t *ptr = (uint8_t *) iData;


  for (int j = 0; j < iLen; j++, ptr++) {
    crc ^= *ptr;
    for (int i = 0; i < 8; i++) {
      if ( crc & 0x80 )
        crc = (uint8_t)((crc << 1) ^ CRCMASK);
      else
        crc <<= 1;
    }
  }
  return crc;
}
#endif


#endif // _PARAMETERS_H_