/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

// This file should be compiled only when the LM_RAND flag is enabled.
//
// The following ifdef prevents XCode and other non-bjam build systems
// from attempting to compile this file when LM_RAND is disabled.
//

#include <limits>
#include <iostream>
#include <fstream>

#include "Rand.h"
#include "moses/Factor.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "RandLM.h"

using namespace std;

namespace Moses
{

LanguageModelRandLM::LanguageModelRandLM(const std::string &line)
  :LanguageModelSingleFactor(line)
  , m_lm(0)
{
}

LanguageModelRandLM::~LanguageModelRandLM()
{
  delete m_lm;
}

void LanguageModelRandLM::Load(AllOptions::ptr const& opts)
{
  cerr << "Loading LanguageModelRandLM..." << endl;
  FactorCollection &factorCollection = FactorCollection::Instance();
  int cache_MB = 50; // increase cache size
  m_lm = randlm::RandLM::initRandLM(m_filePath, m_nGramOrder, cache_MB);
  UTIL_THROW_IF2(m_lm == NULL, "RandLM object not created");
  // get special word ids
  m_oov_id = m_lm->getWordID(m_lm->getOOV());
  CreateFactors(factorCollection);
  m_lm->initThreadSpecificData();
}

void LanguageModelRandLM::CreateFactors(FactorCollection &factorCollection)   // add factors which have randlm id
{
  // code copied & paste from SRI LM class. should do template function
  // first get all bf vocab in map
  std::map<size_t, randlm::WordID> randlm_ids_map; // map from factor id -> randlm id
  size_t maxFactorId = 0; // to create lookup vector later on
  for(std::map<randlm::Word, randlm::WordID>::const_iterator vIter = m_lm->vocabStart();
      vIter != m_lm->vocabEnd(); vIter++) {
    // get word from randlm vocab and associate with (new) factor id
    size_t factorId=factorCollection.AddFactor(Output,m_factorType,vIter->first)->GetId();
    randlm_ids_map[factorId] = vIter->second;
    maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  }
  // add factors for BOS and EOS and store bf word ids
  size_t factorId;
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, m_lm->getBOS());
  factorId = m_sentenceStart->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd	= factorCollection.AddFactor(Output, m_factorType, m_lm->getEOS());
  factorId = m_sentenceEnd->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  // add to lookup vector in object
  m_randlm_ids_vec.resize(maxFactorId+1);
  // fill with OOV code
  fill(m_randlm_ids_vec.begin(), m_randlm_ids_vec.end(), m_oov_id);

  for (map<size_t, randlm::WordID>::const_iterator iter = randlm_ids_map.begin();
       iter != randlm_ids_map.end() ; ++iter)
    m_randlm_ids_vec[iter->first] = iter->second;

}

randlm::WordID LanguageModelRandLM::GetLmID( const std::string &str ) const
{
  return m_lm->getWordID(str);
}

randlm::WordID LanguageModelRandLM::GetLmID( const Factor *factor ) const
{
  size_t factorId = factor->GetId();
  return ( factorId >= m_randlm_ids_vec.size()) ? m_oov_id : m_randlm_ids_vec[factorId];
}

LMResult LanguageModelRandLM::GetValue(const vector<const Word*> &contextFactor,
                                       State* finalState) const
{
  FactorType factorType = GetFactorType();
  // set up context
  randlm::WordID ngram[MAX_NGRAM_SIZE];
  int count = contextFactor.size();
  for (int i = 0 ; i < count ; i++) {
    ngram[i] = GetLmID((*contextFactor[i])[factorType]);
    //std::cerr << m_lm->getWord(ngram[i]) << " ";
  }
  int found = 0;
  LMResult ret;
  ret.score = FloorScore(TransformLMScore(m_lm->getProb(&ngram[0], count, &found, finalState)));
  ret.unknown = count && (ngram[count - 1] == m_oov_id);
  //if (finalState)
  //  std::cerr << " = " << logprob << "(" << *finalState << ", " <<")"<< std::endl;
  //else
  //  std::cerr << " = " << logprob << std::endl;
  return ret;
}

void LanguageModelRandLM::InitializeForInput(ttasksptr const& ttask)
{
  m_lm->initThreadSpecificData(); // Creates thread specific data iff                                    // compiled with multithreading.
}
void LanguageModelRandLM::CleanUpAfterSentenceProcessing(const InputType& source)
{
  m_lm->clearCaches(); // clear caches
}

}

