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

#ifndef moses_PhraseDictionaryCompact_h
#define moses_PhraseDictionaryCompact_h

#include <boost/unordered_map.hpp>

#ifdef WITH_THREADS
#ifdef BOOST_HAS_PTHREADS
#include <boost/thread/mutex.hpp>
#endif
#endif

#include "moses/PhraseDictionary.h"
#include "moses/ThreadPool.h"

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
  bool m_useAlignmentInfo;
  
  typedef std::vector<TargetPhraseCollection*> PhraseCache;
#ifdef WITH_THREADS
  boost::mutex m_sentenceMutex;
  typedef std::map<boost::thread::id, PhraseCache> SentenceCache;
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
                          PhraseDictionaryFeature* feature,
                          bool inMemory = StaticData::Instance().UseMinphrInMemory(),
                          bool useAlignmentInfo = StaticData::Instance().NeedAlignmentInfo())
    : PhraseDictionary(numScoreComponent, feature),
      m_implementation(implementation),
      m_inMemory(inMemory),
      m_useAlignmentInfo(useAlignmentInfo),
      m_hash(10, 16),
      m_phraseDecoder(0),
      m_weight(0)
  {}

  ~PhraseDictionaryCompact();

  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , float weightWP);

  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase &source) const;
  TargetPhraseVectorPtr GetTargetPhraseCollectionRaw(const Phrase &source) const;
  
  void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

  void InitializeForInput(const Moses::InputType&);
  
  void CacheForCleanup(TargetPhraseCollection* tpc);
  void CleanUp(const InputType &source);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &)
  {
    assert(false);
    return 0;
  }

  TO_STRING();

};

}
#endif
