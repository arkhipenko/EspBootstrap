#ifndef _PARAMETERSSPIFFS_H_
#define _PARAMETERSSPIFFS_H_

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
#include <JsonConfigSPIFFS.h>

#define PARAMS_FER  (-6)

class ParametersSPIFFS : public ParametersBase {
  public:
    ParametersSPIFFS(const String& aToken, Dictionary& aDict ) ;
    virtual ~ParametersSPIFFS();

    virtual int8_t  begin();
    virtual int8_t  load();
    virtual int8_t  save();

    void            clear();

  private:

    Dictionary&     iDict;
    String          iFile;
};

ParametersSPIFFS::ParametersSPIFFS(const String& aToken, Dictionary& aDict ) : ParametersBase(aToken), iDict(aDict)  {
  iActive = false;
}


ParametersSPIFFS::~ParametersSPIFFS() {
  if (iActive) {
    save();
    iActive = false;
  }
}


int8_t ParametersSPIFFS::begin() {
  iActive = true;
  iFile = String("/") + iToken + ".json";
#ifdef _LIBDEBUG_
  Serial.printf("ParametersSPIFFS: config file = %s\n", iFile.c_str());
#endif
  return PARAMS_OK;
}


int8_t ParametersSPIFFS::load() {
//  uint16_t iTl = iToken.length();

  if (!iActive) {
    return PARAMS_ACT;
  }
  return JSONConfig.parse(iFile, iDict);
}


int8_t ParametersSPIFFS::save() {
  if (!iActive) {
    return PARAMS_ACT;
  }

  File f = SPIFFS.open(iFile, "w");
  if ( !f ) {
    return PARAMS_FER;
  }

  f.print(iDict.json());
//  int l = iDict.count();
//  f.print('{');
//  for (int i = 0; i < l; i++) {
//    f.print('\"');
//    f.print(iDict(i));
//    f.print("\":\"");
//    f.print(iDict[i]);
//    if ( i < l - 1 ) f.print("\", ");
//  }
//  f.print("\"}");
  f.close();

  return PARAMS_OK;
}


void ParametersSPIFFS::clear () {
  SPIFFS.remove(iFile);
}

#endif // _PARAMETERSSPIFFS_H_