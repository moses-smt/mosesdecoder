//
//  PhraseDictionaryALSuffixArray.h
//  moses
//
//  Created by Hieu Hoang on 06/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_PhraseDictionaryALSuffixArray_h
#define moses_PhraseDictionaryALSuffixArray_h

#include "moses/TranslationModel/PhraseDictionaryMemory.h"

namespace Moses {
  
/** Implementation of in-memory phrase table for use with Adam Lopez's suffix array.
 * Does 2 things that the normal in-memory pt doesn't do:
 *  1. Loads grammar for a sentence to be decoded only when the sentence is being decoded. Unload afterwards
    2. Format of the pt file follows Hiero, rather than Moses
 */   
class PhraseDictionaryALSuffixArray : public PhraseDictionarySCFG
{
public:
  PhraseDictionaryALSuffixArray(const std::string &line)
  : PhraseDictionarySCFG("PhraseDictionaryALSuffixArray", line)
  {}

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , size_t tableLimit);

  void InitializeForInput(InputType const& source);

protected:
  const std::vector<FactorType> *m_input, *m_output;
  
};
  

}

#endif
