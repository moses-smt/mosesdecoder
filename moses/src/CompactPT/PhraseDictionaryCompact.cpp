// $Id: PhraseDictionaryMemoryHashed.cpp 3908 2011-02-28 11:41:08Z pjwilliams $
// vim:tabstop=2

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

#include <fstream>
#include <string>
#include <iterator>
#include <queue>
#include <algorithm>
#include <sys/stat.h>

#include "PhraseDictionaryCompact.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "ThreadPool.h"

using namespace std;

namespace Moses
{
  
bool PhraseDictionaryCompact::Load(const std::vector<FactorType> &input
                                  , const std::vector<FactorType> &output
                                  , const string &filePath
                                  , const vector<float> &weight
                                  , size_t tableLimit
                                  , const LMList &languageModels
                                  , float weightWP)
{
  m_input = &input;
  m_output = &output;
  m_weight = &weight;
  m_tableLimit = tableLimit;
  m_languageModels = &languageModels; 
  m_weightWP = weightWP;

  std::string fullFilePath = filePath;

  m_phraseDecoder = new PhraseDecoder(*this, m_input, m_output, m_feature,
                                  m_numScoreComponent, m_weight, m_weightWP,
                                  m_languageModels);

  std::FILE* pFile = std::fopen(fullFilePath.c_str() , "r");
  
  size_t indexSize;
  if(m_inMemory)
    // Load source phrase index into memory
    indexSize = m_hash.Load(pFile);
  else
    // Keep source phrase index on disk
    indexSize = m_hash.LoadIndex(pFile);

  
  size_t coderSize = m_phraseDecoder->Load(pFile);
  
  size_t phraseSize;
  if(m_inMemory)
    // Load target phrase collections into memory
    phraseSize = m_targetPhrasesMemory.load(pFile, false);
  else
    // Keep target phrase collections on disk
    phraseSize = m_targetPhrasesMapped.load(pFile, true);
  
  return indexSize && coderSize && phraseSize;    
}

struct CompareTargetPhrase {
  bool operator() (const TargetPhrase &a, const TargetPhrase &b) {
    return a.GetFutureScore() > b.GetFutureScore();
  }
};

const TargetPhraseCollection*
PhraseDictionaryCompact::GetTargetPhraseCollection(const Phrase &sourcePhrase) const {
  
  // There is no souch source phrase if source phrase is longer than longest
  // observed source phrase during compilation 
  if(sourcePhrase.GetSize() > m_phraseDecoder->GetMaxSourcePhraseLength())
    return NULL;

  // Retrieve target phrase collection from phrase table
  TargetPhraseVectorPtr decodedPhraseColl
    = m_phraseDecoder->CreateTargetPhraseCollection(sourcePhrase, true);
  
  if(decodedPhraseColl != NULL && decodedPhraseColl->size()) {
    TargetPhraseVectorPtr tpv(new TargetPhraseVector(*decodedPhraseColl));
    TargetPhraseCollection* phraseColl = new TargetPhraseCollection();
    
    // Score phrases and if possible apply ttable_limit
    TargetPhraseVector::iterator nth =
      (m_tableLimit == 0 || tpv->size() < m_tableLimit) ?
      tpv->end() : tpv->begin() + m_tableLimit;
    std::nth_element(tpv->begin(), nth, tpv->end(), CompareTargetPhrase());
    for(TargetPhraseVector::iterator it = tpv->begin(); it != nth; it++)
      phraseColl->Add(new TargetPhrase(*it));
    
    // Cache phrase pair for for clean-up or retrieval with PREnc
    const_cast<PhraseDictionaryCompact*>(this)->CacheForCleanup(sourcePhrase, phraseColl);
    
    return phraseColl;
  }
  else
    return NULL;
  
}

PhraseDictionaryCompact::~PhraseDictionaryCompact() {
  if(m_phraseDecoder)
    delete m_phraseDecoder;
}

//TO_STRING_BODY(PhraseDictionaryCompact)

TargetPhraseCollection*
PhraseDictionaryCompact::RetrieveFromCache(const Phrase &sourcePhrase) {
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_sentenceMutex);
  PhraseCache &ref = m_sentenceCache[pthread_self()]; 
#else
  PhraseCache &ref = m_sentenceCache; 
#endif
  PhraseCache::iterator it = ref.find(sourcePhrase);
  if(it != ref.end())
    return it->second;
  else
    return NULL;
}

void PhraseDictionaryCompact::CacheForCleanup(const Phrase &sourcePhrase,
                                                   TargetPhraseCollection* tpc) {
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_sentenceMutex);
  m_sentenceCache[pthread_self()].insert(std::make_pair(sourcePhrase, tpc));
#else
  m_sentenceCache.insert(std::make_pair(sourcePhrase, tpc));
#endif
}

void PhraseDictionaryCompact::InitializeForInput(const Moses::InputType&) {}

void PhraseDictionaryCompact::AddEquivPhrase(const Phrase &source,
                                             const TargetPhrase &targetPhrase) { }

void PhraseDictionaryCompact::CleanUp() {
  if(!m_inMemory)
    m_hash.KeepNLastRanges(0.01, 0.2);
    
  m_phraseDecoder->PruneCache();
  
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_sentenceMutex);
  PhraseCache &ref = m_sentenceCache[pthread_self()]; 
#else
  PhraseCache &ref = m_sentenceCache; 
#endif
  
  for(PhraseCache::iterator it = ref.begin(); it != ref.end(); it++) 
      delete it->second;
      
  PhraseCache temp;
  temp.swap(ref);
}

}

