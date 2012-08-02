
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

#ifndef moses_PhraseDictionaryCompact_h
#define moses_PhraseDictionaryCompact_h

#include <boost/unordered_map.hpp>

#ifdef WITH_THREADS
#ifdef BOOST_HAS_PTHREADS
#include <boost/thread/mutex.hpp>
#endif
#endif

#include "PhraseDictionary.h"
#include "ThreadPool.h"

#include "BlockHashIndex.h"
#include "StringVector.h"
#include "PhraseDecoder.h"
#include "TargetPhraseCollectionCache.h"

namespace Moses
{

class PhraseDecoder;

class PhraseDictionaryCompact : public PhraseDictionary
{
protected:
  friend class PhraseDecoder;

  PhraseTableImplementation m_implementation;
  bool m_inMemory;
  
  typedef std::map<Phrase, TargetPhraseCollection*> PhraseCache;
#ifdef WITH_THREADS
  boost::mutex m_sentenceMutex;
  typedef std::map<size_t, PhraseCache>  SentenceCache;
#else
  typedef PhraseCache SentenceCache;
#endif
  SentenceCache m_sentenceCache;
  
  BlockHashIndex m_hash;
  PhraseDecoder* m_phraseDecoder;
  
  StringVector<unsigned char, size_t, MmapAllocator>  m_targetPhrasesMapped;
  StringVector<unsigned char, size_t, std::allocator> m_targetPhrasesMemory;
  
  const std::vector<FactorType>* m_input;
  const std::vector<FactorType>* m_output;
  
  const std::vector<float>* m_weight;
  const LMList* m_languageModels;
  float m_weightWP;

public:
  PhraseDictionaryCompact(size_t numScoreComponent,
                               PhraseTableImplementation implementation,
                               PhraseDictionaryFeature* feature)
    : PhraseDictionary(numScoreComponent, feature),
      m_implementation(implementation),
      m_inMemory(StaticData::Instance().UseMinphrInMemory()),
      m_hash(10, 16),
      m_phraseDecoder(0)
  {}
    
  virtual ~PhraseDictionaryCompact();

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , float weightWP);

  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase &source) const;

  void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

  void InitializeForInput(const Moses::InputType&);
  
  TargetPhraseCollection* RetrieveFromCache(const Phrase &sourcePhrase);
  void CacheForCleanup(const Phrase &source, TargetPhraseCollection* tpc);
  void CleanUp();

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollection &)
  {
    assert(false);
    return 0;
  }

  TO_STRING();

};

}
#endif
