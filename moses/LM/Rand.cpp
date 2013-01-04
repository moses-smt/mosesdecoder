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

#include <limits>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "SingleFactor.h"
#include "RandLM.h"
#include "Rand.h"
#include "moses/Factor.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "util/check.hh"


namespace Moses
{
namespace 
{
using namespace std;

class LanguageModelRandLM : public LanguageModelPointerState
{
public:
  LanguageModelRandLM()
    : m_lm(0) {}
  bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);
  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;
  ~LanguageModelRandLM() {
    delete m_lm;
  }
  void CleanUpAfterSentenceProcessing() {
    m_lm->clearCaches(); // clear caches
  }
  void InitializeBeforeSentenceProcessing() {
    m_lm->initThreadSpecificData(); // Creates thread specific data iff
                                    // compiled with multithreading.
  }
protected:
  std::vector<randlm::WordID> m_randlm_ids_vec;
  randlm::RandLM* m_lm;
  randlm::WordID m_oov_id;
  void CreateFactors(FactorCollection &factorCollection);
  randlm::WordID GetLmID( const std::string &str ) const;
  randlm::WordID GetLmID( const Factor *factor ) const {
    size_t factorId = factor->GetId();
    return ( factorId >= m_randlm_ids_vec.size()) ? m_oov_id : m_randlm_ids_vec[factorId];
  };

};


bool LanguageModelRandLM::Load(const std::string &filePath, FactorType factorType,
                               size_t nGramOrder)
{
  cerr << "Loading LanguageModelRandLM..." << endl;
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_filePath = filePath;
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  int cache_MB = 50; // increase cache size
  m_lm = randlm::RandLM::initRandLM(filePath, nGramOrder, cache_MB);
  CHECK(m_lm != NULL);
  // get special word ids
  m_oov_id = m_lm->getWordID(m_lm->getOOV());
  CreateFactors(factorCollection);
  m_lm->initThreadSpecificData();
  return true;
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
  m_sentenceStartArray[m_factorType] = m_sentenceStart;

  m_sentenceEnd	= factorCollection.AddFactor(Output, m_factorType, m_lm->getEOS());
  factorId = m_sentenceEnd->GetId();
  maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
  m_sentenceEndArray[m_factorType] = m_sentenceEnd;

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

}

LanguageModelPointerState *NewRandLM() {
  return new LanguageModelRandLM();
}

}

