//
//  PhraseDictionaryHiero.h
//  moses
//
//  Created by Hieu Hoang on 04/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_PhraseDictionaryHiero_h
#define moses_PhraseDictionaryHiero_h

#include "PhraseDictionarySCFG.h"

namespace Moses {

class PhraseDictionaryHiero : public PhraseDictionarySCFG
{
public:
  PhraseDictionaryHiero(size_t numScoreComponent, PhraseDictionaryFeature* feature)
  : PhraseDictionarySCFG(numScoreComponent,feature) {}

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer);

};

}

#endif
