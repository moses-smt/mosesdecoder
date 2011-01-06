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

#include <cassert>
#include <limits>
#include <iostream>
#include <fstream>

#include "LanguageModelRandLM.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"

namespace Moses
{
using namespace std;

bool LanguageModelRandLM::Load(const std::string &filePath, FactorType factorType, 
			       size_t nGramOrder) {
  cerr << "Loading LanguageModelRandLM..." << endl;
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_filePath = filePath;
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  int cache_MB = 50; // increase cache size
  m_lm = randlm::RandLM::initRandLM(filePath, nGramOrder, cache_MB);
  assert(m_lm != NULL);
  // get special word ids
  m_oov_id = m_lm->getWordID(m_lm->getOOV());
  CreateFactors(factorCollection);
  return true;
}

void LanguageModelRandLM::CreateFactors(FactorCollection &factorCollection) { // add factors which have randlm id
  // code copied & paste from SRI LM class. should do template function
  // first get all bf vocab in map
  std::map<size_t, randlm::WordID> randlm_ids_map; // map from factor id -> randlm id
  size_t maxFactorId = 0; // to create lookup vector later on
  for(std::map<randlm::Word, randlm::WordID>::const_iterator vIter = m_lm->vocabStart();
      vIter != m_lm->vocabEnd(); vIter++){
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

randlm::WordID LanguageModelRandLM::GetLmID( const std::string &str ) const {
  return m_lm->getWordID(str);
}

float LanguageModelRandLM::GetValue(const vector<const Word*> &contextFactor,
				    State* finalState, unsigned int* len) const {
  unsigned int dummy;   // is this needed ?
  if (!len) { len = &dummy; }
  FactorType factorType = GetFactorType();
  // set up context
  randlm::WordID ngram[MAX_NGRAM_SIZE];
  int count = contextFactor.size();
  for (int i = 0 ; i < count ; i++) {
    ngram[i] = GetLmID((*contextFactor[i])[factorType]);
    //std::cerr << m_lm->getWord(ngram[i]) << " ";
  }
  int found = 0;
  float logprob = FloorScore(TransformLMScore(m_lm->getProb(&ngram[0], count, &found, finalState)));
  *len = 0; // not available
  //if (finalState)
  //  std::cerr << " = " << logprob << "(" << *finalState << ", " << *len <<")"<< std::endl;
  //else
  //  std::cerr << " = " << logprob << std::endl;
  return logprob;
}

}


