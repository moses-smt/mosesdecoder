//
//  PhraseDictionaryALSuffixArray.cpp
//  moses
//
//  Created by Hieu Hoang on 06/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "PhraseDictionaryALSuffixArray.h"
#include "moses/InputType.h"
#include "moses/InputFileStream.h"
#include "moses/TypeDef.h"
#include "moses/TranslationTask.h"
#include "moses/StaticData.h"
#include "Loader.h"
#include "LoaderFactory.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
PhraseDictionaryALSuffixArray::PhraseDictionaryALSuffixArray(const std::string &line)
  : PhraseDictionaryMemory(1, line)
{
  const StaticData &staticData = StaticData::Instance();
  if (staticData.ThreadCount() > 1) {
    throw runtime_error("Suffix array implementation is not threadsafe");
  }

  ReadParameters();
}

void PhraseDictionaryALSuffixArray::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
}

void PhraseDictionaryALSuffixArray::InitializeForInput(ttasksptr const& ttask)
{
  InputType const& source = *ttask->GetSource();
  // populate with rules for this sentence
  long translationId = source.GetTranslationId();

  string grammarFile = GetFilePath() + "/grammar." + SPrint(translationId) + ".gz";

  std::auto_ptr<RuleTableLoader> loader =
    RuleTableLoaderFactory::Create(grammarFile);
  AllOptions::ptr const& opts = ttask->options();
  bool ret = loader->Load(*opts, m_input, m_output, grammarFile, m_tableLimit, *this);

  UTIL_THROW_IF2(!ret, "Rules not successfully loaded for sentence id "
                 << translationId);
}

void PhraseDictionaryALSuffixArray::CleanUpAfterSentenceProcessing(const InputType &source)
{
  m_collection.Remove();
}

}
