//
//  PhraseDictionaryHiero.cpp
//  moses
//
//  Created by Hieu Hoang on 04/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "PhraseDictionaryHiero.h"
#include "PhraseDictionarySCFG.h"
#include "InputFileStream.h"
#include "RuleTableLoader.h"
#include "RuleTableLoaderFactory.h"

using namespace std;

namespace Moses {

bool PhraseDictionaryHiero::Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , const WordPenaltyProducer* wpProducer)
{
  m_filePath = filePath;
  m_tableLimit = tableLimit;
  
  
  // data from file
  InputFileStream inFile(filePath);
  
  std::auto_ptr<RuleTableLoader> loader =
  RuleTableLoaderFactory::Create(filePath);
  bool ret = loader->Load(input, output, inFile, weight, tableLimit,
                          languageModels, wpProducer, *this);
  return ret;
}
  
} // namespace


