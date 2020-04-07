#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_


#include <Arduino.h>
#include <EEPROM.h>

#ifndef EEPROM_MAX
#if defined( ARDUINO_ARCH_AVR )
#define EEPROM_MAX  512
#endif

#if defined( ARDUINO_ARCH_ESP8266 )
#define EEPROM_MAX  4096
#endif
#endif // #ifndef EEPROM_MAX


// Error codes:
#define PARAMS_OK   0
#define PARAMS_ERR  (-1)
#define PARAMS_LEN  (-2)
#define PARAMS_CRC  (-3)
#define PARAMS_TOK  (-4)
#define PARAMS_ACT  (-99)

template<typename T>
class Parameters {
  public:
    Parameters(uint16_t address, const String& token, T* ptr, T* deflt = NULL, uint16_t maxlength = EEPROM_MAX );
    ~Parameters();

    inline int8_t   lastError() {return iRc;}
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
  if ( iMaxLen >= EEPROM_MAX-1 ) iMaxLen = EEPROM_MAX-1;
  if ( iLen+1 <= iMaxLen ) {
#if defined( ARDUINO_ARCH_ESP8266 )
    EEPROM.begin(iLen+1);
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
#if defined( ARDUINO_ARCH_ESP8266 )
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
  
  for (uint16_t i=0; i<iLen; i++, ptr++) {
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
  for (uint16_t i=0; i<iLen; i++, ptr++) {
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
#if defined( ARDUINO_ARCH_ESP8266 )
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

#endif // _PARAMETERS_H_