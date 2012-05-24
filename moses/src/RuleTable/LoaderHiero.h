//
//  RuleTableLoaderHiero.h
//  moses
//
//  Created by Hieu Hoang on 04/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_RuleTableLoaderHiero_h
#define moses_RuleTableLoaderHiero_h

#include "RuleTable/LoaderStandard.h"

namespace Moses {

class RuleTableLoaderHiero : public RuleTableLoaderStandard
{
public:
  bool Load(const std::vector<FactorType> &input,
            const std::vector<FactorType> &output,
            std::istream &inStream,
            const std::vector<float> &weight,
            size_t tableLimit,
            const LMList &languageModels,
            const WordPenaltyProducer* wpProducer,
            RuleTableTrie &);

};

}

#endif
