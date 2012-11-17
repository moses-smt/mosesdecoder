// $Id$                                                                                                                               
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
#include "moses/FactorCollection.h"
#include "moses/Word.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"
#include "moses/UserMessage.h"
#include "moses/ThreadPool.h"

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
  m_weight = new std::vector<float>(weight);
  m_tableLimit = tableLimit;
  m_languageModels = &languageModels; 
  m_weightWP = weightWP;
 
  std::string tFilePath = filePath;
  
  std::string suffix = ".minphr";
  if(tFilePath.substr(tFilePath.length() - suffix.length(), suffix.length()) == suffix)
  {
    if(!FileExists(tFilePath))
    {
      std::cerr << "Error: File " << tFilePath << " does not exit." << std::endl;
      exit(1);
    }
  }
  else 
  {
    if(FileExists(tFilePath + suffix))
    {
      tFilePath += suffix;
    }
    else
    {
       std::cerr << "Error: File " << tFilePath << ".minphr does not exit." << std::endl;
       exit(1);
    }
  }

  m_phraseDecoder = new PhraseDecoder(*this, m_input, m_output, m_feature,
                                  m_numScoreComponent, m_weight, m_weightWP,
                                  m_languageModels);

  std::FILE* pFile = std::fopen(tFilePath.c_str() , "r");
  
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
    const_cast<PhraseDictionaryCompact*>(this)->CacheForCleanup(phraseColl);
    
    return phraseColl;
  }
  else
    return NULL;
}

TargetPhraseVectorPtr
PhraseDictionaryCompact::GetTargetPhraseCollectionRaw(const Phrase &sourcePhrase) const {

  // There is no souch source phrase if source phrase is longer than longest
  // observed source phrase during compilation 
  if(sourcePhrase.GetSize() > m_phraseDecoder->GetMaxSourcePhraseLength())
    return TargetPhraseVectorPtr();

  // Retrieve target phrase collection from phrase table
  return m_phraseDecoder->CreateTargetPhraseCollection(sourcePhrase, true);
}

PhraseDictionaryCompact::~PhraseDictionaryCompact() {
  if(m_phraseDecoder)
    delete m_phraseDecoder;
  if(m_weight)
    delete m_weight;
}

//TO_STRING_BODY(PhraseDictionaryCompact)

void PhraseDictionaryCompact::CacheForCleanup(TargetPhraseCollection* tpc) {
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_sentenceMutex);
  PhraseCache &ref = m_sentenceCache[boost::this_thread::get_id()]; 
#else
  PhraseCache &ref = m_sentenceCache; 
#endif
  ref.push_back(tpc);
}

void PhraseDictionaryCompact::InitializeForInput(const Moses::InputType&) {}

void PhraseDictionaryCompact::AddEquivPhrase(const Phrase &source,
                                             const TargetPhrase &targetPhrase) { }

void PhraseDictionaryCompact::CleanUp(const InputType &source) {
  if(!m_inMemory)
    m_hash.KeepNLastRanges(0.01, 0.2);
    
  m_phraseDecoder->PruneCache();
  
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_sentenceMutex);
  PhraseCache &ref = m_sentenceCache[boost::this_thread::get_id()]; 
#else
  PhraseCache &ref = m_sentenceCache; 
#endif
  
  for(PhraseCache::iterator it = ref.begin(); it != ref.end(); it++) 
      delete *it;
      
  PhraseCache temp;
  temp.swap(ref);
}

}

