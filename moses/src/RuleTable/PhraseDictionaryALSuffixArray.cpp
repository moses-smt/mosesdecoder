//
//  PhraseDictionaryALSuffixArray.cpp
//  moses
//
//  Created by Hieu Hoang on 06/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "PhraseDictionaryALSuffixArray.h"
#include "InputType.h"
#include "InputFileStream.h"
#include "RuleTable/Loader.h"
#include "RuleTable/LoaderFactory.h"
#include "TypeDef.h"
#include "StaticData.h"
#include "UserMessage.h"

using namespace std;

namespace Moses 
{
  
bool PhraseDictionaryALSuffixArray::Load(const std::vector<FactorType> &input
                                 , const std::vector<FactorType> &output
                                 , const std::string &filePath
                                 , const std::vector<float> &weight
                                 , size_t tableLimit
                                 , const LMList &languageModels
                                 , const WordPenaltyProducer* wpProducer)
{
  const StaticData &staticData = StaticData::Instance();
  if (staticData.ThreadCount() > 1)
  {
    UserMessage::Add("Suffix array implementation is not threadsafe");
    return false;
  }
  
  // file path is the directory of the rules for eacg, NOT the file of all the rules
  SetFilePath(filePath);
  m_tableLimit = tableLimit;

  m_input = &input;
  m_output = &output;
  m_languageModels = &languageModels;
  m_wpProducer = wpProducer;
  m_weight = &weight;
  
  return true;
}

void PhraseDictionaryALSuffixArray::InitializeForInput(InputType const& source)
{
  // clear out rules for previous sentence
  m_collection.Clear();
  
  // populate with rules for this sentence
  long translationId = source.GetTranslationId();
  
  string grammarFile = GetFilePath() + "/grammar.out." + SPrint(translationId) + ".gz";
  
  // data from file
  InputFileStream inFile(grammarFile);

  std::auto_ptr<RuleTableLoader> loader =
  RuleTableLoaderFactory::Create(grammarFile);
  bool ret = loader->Load(*m_input, *m_output, inFile, *m_weight, m_tableLimit,
                          *m_languageModels, m_wpProducer, *this);
  
  CHECK(ret);
}

}
