#ifndef _PARAMETERSEEPROM_H_
#define _PARAMETERSEEPROM_H_

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
#include <Dictionary.h>
#include <EEPROM.h>

#ifndef EEPROM_MAX

#if defined( ARDUINO_ARCH_AVR )
#define EEPROM_MAX  512
#endif

#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
#define EEPROM_MAX  4096
#endif

#ifndef EEPROM_MAX
#define EEPROM_MAX  256 // safe default
#endif

#endif // #ifndef EEPROM_MAX

class ParametersEEPROM : public ParametersBase {
public:
  ParametersEEPROM(const String & aToken, Dictionary & aDict, uint16_t aAddress, uint16_t aSize ) ;
  virtual ~ParametersEEPROM();

  virtual int8_t  begin();
  virtual int8_t  load();
  virtual int8_t  save();
  
  void            clear();

private:
  uint8_t         checksum ();

  Dictionary&     iDict;
  uint16_t        iAddress;
  uint8_t*        iData;
  uint16_t        iSize;
};

ParametersEEPROM::ParametersEEPROM(const String& aToken, Dictionary& aDict, uint16_t aAddress, uint16_t aSize ) : ParametersBase(aToken), iDict(aDict)  {
  iActive = false;
  iAddress = aAddress;
  iSize = aSize;
  iData = NULL;
}


ParametersEEPROM::~ParametersEEPROM() {
  if (iActive) {
    save();
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.end();
#endif
    iActive = false;
  }
}


int8_t ParametersEEPROM::begin() {
  uint16_t maxLen = iToken.length() + iDict.esize() + 4; // 4: 1 null for token, 1 crc8, 2 bytes for count
  if ( iSize < EEPROM_MAX && maxLen <= iSize) {
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
    EEPROM.begin(4096); // allocate all memory
#endif
    iActive = true;
    return PARAMS_OK;
  }
  else {
    return PARAMS_LEN;
  }
}


int8_t ParametersEEPROM::load() {
  uint16_t iTl = iToken.length();

  if (!iActive) {
    return PARAMS_ACT;
  }

  iData = (uint8_t * ) malloc(iSize);
  if (iData == NULL) {
    //    iRc = PARAMS_MEM;
    return PARAMS_MEM;
  }
  uint8_t* p = iData;

  for (uint16_t i = 0; i < iSize - 1; i++, p++) {
    *p = EEPROM.read(iAddress + i);
  }
  uint8_t crc = EEPROM.read( iAddress + iSize - 1);

  // Check CRC
  if (crc != checksum () ) {
    free(iData);
    iData = NULL;
    return PARAMS_CRC;
  }

  // Check Token
  if ( strncmp( (const char *) iToken.c_str(), (const char *) iData, iSize ) != 0 ) {
    free(iData);
    iData = NULL;
    return PARAMS_TOK;
  }

  // Populate the dictionary
  p = iData + (iTl + 1);

  uint16_t cnt = *p | ((((uint16_t) * (p + 1)) << 8) & 0xff00);
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
  return PARAMS_OK;
  //  if ( iMode == PARAMS_FILE ) {
  //    String file = "/" + iToken + ".json";
  //    if ( !SPIFFS.exists(file) ) {
  //      return PARAMS_FDE;
  //    }
  //  }
}


int8_t ParametersEEPROM::save() {
  uint8_t changed = 0;
  int8_t  rc = PARAMS_OK;
  
  if (!iActive) {
    return PARAMS_ACT;
  }

  uint16_t iTl = iToken.length();
  uint16_t iDs = iDict.esize();
  uint16_t iDc = iDict.count();
  uint16_t maxLen = iTl + iDs + 4;

  if ( maxLen >= iSize ) {
    return PARAMS_LEN;
  }
  iData = (uint8_t * ) malloc(iSize);
  if (iData == NULL) {
    return PARAMS_MEM;
  }
  clear();
  uint8_t* p = iData;

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: memory allocated and cleared");
  Serial.print("Token value: ");
  Serial.println(iToken);
  Serial.println(iToken.c_str());
#endif

  strcpy((char*)p, iToken.c_str());
  p += (iTl + 1);

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: token copied");
#endif

  *p++ = iDc & 0xff;
  *p++ = (iDc >> 8) & 0xff;

  for (uint16_t i = 0; i < iDc; i++) {
    strcpy((char*)p, iDict(i).c_str());
    p += (iDict(i).length() + 1);
    strcpy((char*)p, iDict[i].c_str());
    p += (iDict[i].length() + 1);

#ifdef _LIBDEBUG_
    Serial.printf ("Parameters save: k-v pair #%d copies: %s : %s\n", i, iDict(i).c_str(), iDict[i].c_str());
#endif

  }

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: Key-value pairs copied");
#endif


  p = iData;

  for (uint16_t i = 0; i < iSize - 1; i++, p++) {
#if defined( ARDUINO_ARCH_AVR )
    EEPROM.update(iAddress + i, *p);
#else
    // protect memory from excessive writes
    uint8_t b = EEPROM.read(iAddress + i);
    if ( b != *p) {
        EEPROM.write(iAddress + i, *p);
        changed = 1;
    }
#endif
  }

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: EEPROM updated");
#endif


#if defined( ARDUINO_ARCH_AVR )
  EEPROM.update( iAddress + iSize - 1, checksum () );
#else
  {
    uint8_t b = EEPROM.read(iAddress + iSize - 1);
    uint8_t c = checksum ();
    if ( b != c ) { 
        EEPROM.write( iAddress + iSize - 1, c );
        changed = 1;
    }
  }
#endif
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
  if ( changed ) {
    if ( !EEPROM.commit() ) rc = PARAMS_ERR; 
  }
#endif

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: checksum saved");
#endif


  free(iData);
  iData = NULL;

#ifdef _LIBDEBUG_
  Serial.println ("Parameters save: memory freed");
#endif

  return rc;

}


void ParametersEEPROM::clear () {
  if (iData) {
    memset((void *) iData, 0, iSize - 1);
  }
}


#define CRCMASK 0x1d
uint8_t ParametersEEPROM::checksum () {
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

#endif // _PARAMETERSEEPROM_H_