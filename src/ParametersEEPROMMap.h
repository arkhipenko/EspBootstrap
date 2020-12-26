#ifndef _PARAMETERSEEPROMMAP_H_
#define _PARAMETERSEEPROMMAP_H_

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

#include <ParametersBase.h>
#include <EEPROM.h>

#ifndef EEPROM_MAX

#define EEPROM_MAX  256 // safe default

#if defined( ARDUINO_ARCH_AVR )
#define EEPROM_MAX  512
#endif

#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
#define EEPROM_MAX  4096
#endif

#endif // #ifndef EEPROM_MAX

class ParametersEEPROMMap : public ParametersBase {
  public:
    ParametersEEPROMMap( const String& aToken, void* aPtr, void* aDeflt = NULL, uint16_t aAddress = 0, uint16_t aLength = (EEPROM_MAX-1) );
    virtual ~ParametersEEPROMMap();

    virtual int8_t  begin();
    virtual int8_t  load();
    virtual int8_t  save();
    
    void            loadDefaults();
    void            clear();

  private:
    uint8_t         checksum ();

    void*           iData;
    void*           iDefault;
    uint16_t        iAddress;
    uint16_t        iLen;
    uint16_t        iMaxLen;

};

ParametersEEPROMMap::ParametersEEPROMMap(const String& aToken, void* aPtr, void* aDeflt, uint16_t aAddress, uint16_t aLength ) : ParametersBase(aToken)  {
  iActive = false;
  iAddress = aAddress;

  iData = aPtr;
  iDefault = aDeflt;
  iLen = aLength;
  iMaxLen = iLen + 1; // to account for 1 additional crc byte
}


ParametersEEPROMMap::~ParametersEEPROMMap() {
  if (iActive) {
    save();
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.end();
#endif
    iActive = false;
  }
}


int8_t ParametersEEPROMMap::begin() {

  if ( iMaxLen < 4 ) iMaxLen = 4;
  if ( iMaxLen <= EEPROM_MAX ) {
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.begin(4096);
#endif
    iActive = true;
    return PARAMS_OK;
  }
  else {
    return PARAMS_LEN;
  }
}


int8_t ParametersEEPROMMap::load() {
  uint8_t *ptr = (uint8_t *) iData;

  if (!iActive) {
    return PARAMS_ACT;
  }

  for (uint16_t i = 0; i < iLen; i++, ptr++) {
    *ptr = EEPROM.read(iAddress + i);
  }
  uint8_t crc = EEPROM.read( iAddress + iLen);

  if (crc != checksum () ) {
    loadDefaults();
    return PARAMS_CRC;
  }

  if ( strncmp( (const char *) iToken.c_str(), (const char *) iData, iMaxLen ) != 0 ) {
    loadDefaults();
    return PARAMS_TOK;
  }
  return PARAMS_OK;
}


int8_t ParametersEEPROMMap::save() {
  uint8_t *ptr = (uint8_t *) iData;
  uint8_t changed = 0;
  int8_t  rc = PARAMS_OK;
  
  if (!iActive) {
    return PARAMS_ACT;
  }

//  if ( iLen >= iMaxLen ) {
//    return PARAMS_LEN;
//  }
  for (uint16_t i = 0; i < iLen; i++, ptr++) {
#if defined( ARDUINO_ARCH_AVR )
    EEPROM.update(iAddress + i, *ptr);
#else
    // protect memory from excessive writes
    uint8_t b = EEPROM.read(iAddress + i);
    if ( b != *ptr) {
        EEPROM.write(iAddress + i, *ptr);
        changed = 1;
    }
#endif
  }
#if defined( ARDUINO_ARCH_AVR )
  EEPROM.update( iAddress + iLen, checksum () );
#else
  {
    uint8_t b = EEPROM.read(iAddress + iLen);
    uint8_t c = checksum ();
    if ( b != c ) { 
        EEPROM.write( iAddress + iLen, c );
        changed = 1;
    }
  }
//  EEPROM.write( iAddress + iLen, checksum () );
#endif
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
  if ( changed ) {
    if ( !EEPROM.commit() ) rc = PARAMS_ERR;
  }
#endif  

  return rc;
}


void ParametersEEPROMMap::loadDefaults () {
  if ( iDefault ) {
    memcpy(iData, iDefault, iLen);
  }
  else {
    clear();
  }
  strcpy((char *) iData, iToken.c_str());
}


void ParametersEEPROMMap::clear () {
  memset( iData, 0, iLen );
}


#define CRCMASK 0x1d
uint8_t ParametersEEPROMMap::checksum () {
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

#endif //  _PARAMETERSEEPROMMAP_H_
