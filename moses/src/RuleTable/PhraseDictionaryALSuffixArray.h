//
//  PhraseDictionaryALSuffixArray.h
//  moses
//
//  Created by Hieu Hoang on 06/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_PhraseDictionaryALSuffixArray_h
#define moses_PhraseDictionaryALSuffixArray_h

#include "PhraseDictionarySCFG.h"

namespace Moses {
  
/** Implementation of in-memory phrase table for use with Adam Lopez's suffix array.
 * Does 2 things that the normal in-memory pt doesn't do:
 *  1. Loads grammar for a sentence to be decoded only when the sentence is being decoded. Unload afterwards
    2. Format of the pt file follows Hiero, rather than Moses
 */   
class PhraseDictionaryALSuffixArray : public PhraseDictionarySCFG
{
public:
  PhraseDictionaryALSuffixArray(size_t numScoreComponent, PhraseDictionaryFeature* feature)
  : PhraseDictionarySCFG(numScoreComponent,feature) {}

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer);

  void InitializeForInput(InputType const& source);

protected:
  const std::vector<FactorType> *m_input, *m_output;
  const LMList *m_languageModels;
  const WordPenaltyProducer *m_wpProducer;
  const std::vector<float> *m_weight;
  
};
  

}

#endif
