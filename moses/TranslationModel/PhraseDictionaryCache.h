/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#pragma once

#ifndef moses_PhraseDictionaryCache_H
#define moses_PhraseDictionaryCache_H

#include "moses/TypeDef.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationTask.h"

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_io.hpp>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

#define CBTM_SCORE_TYPE_UNDEFINED (-1)
#define CBTM_SCORE_TYPE_HYPERBOLA 0
#define CBTM_SCORE_TYPE_POWER 1
#define CBTM_SCORE_TYPE_EXPONENTIAL 2
#define CBTM_SCORE_TYPE_COSINE 3
#define CBTM_SCORE_TYPE_HYPERBOLA_REWARD 10
#define CBTM_SCORE_TYPE_POWER_REWARD 11
#define CBTM_SCORE_TYPE_EXPONENTIAL_REWARD 12
#define PI 3.14159265


namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;
class TranslationTask;
class PhraseDictionary;

/** Implementation of a Cache-based phrase table.
 */
class PhraseDictionaryCache : public PhraseDictionary
{

  typedef std::pair<TargetPhraseCollection::shared_ptr, Scores*> TargetCollectionPair;
  typedef boost::unordered_map<Phrase, TargetCollectionPair> cacheMap;
  typedef std::map<long, cacheMap> sentCacheMap;

  // factored translation
  std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;

  // data structure for the cache
  sentCacheMap m_cacheTM;
  long m_sentences;
  unsigned int m_numscorecomponent;
  size_t m_score_type; //scoring type of the match
  size_t m_entries; //total number of entries in the cache
  float m_lower_score; //lower_bound_score for no match
  bool m_constant; //flag for setting a non-decaying cache
  std::string m_initfiles; // vector of files loaded in the initialization phase
  std::string m_name; // internal name to identify this instance of the Cache-based phrase table

#ifdef WITH_THREADS
  //multiple readers - single writer lock
  mutable boost::shared_mutex m_cacheLock;
#endif

  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryCache&);

public:
  PhraseDictionaryCache(const std::string &line);
  ~PhraseDictionaryCache();

  inline const std::string GetName() {
    return m_name;
  };
  inline void SetName(const std::string name) {
    m_name = name;
  }

  static const PhraseDictionaryCache* Instance(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }

  static PhraseDictionaryCache* InstanceNonConst(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }


  static const PhraseDictionaryCache& Instance() {
    return *s_instance;
  }

  static PhraseDictionaryCache& InstanceNonConst() {
    return *s_instance;
  }

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollectionLEGACY(ttasksptr const& ttask,
                                  Phrase const& src) const {
    GetTargetPhraseCollection(src, ttask->GetSource()->GetTranslationId());
  }


  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection(const Phrase &src, long tID) const;

  void CleanUpAfterSentenceProcessing(const InputType& source);
  // for phrase-based model
//  virtual void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  void SetParameter(const std::string& key, const std::string& value);

  void InitializeForInput(ttasksptr const& ttask);

  void Print() const;             // prints the cache
  void Clear();           // clears the cache
  void Clear(long tID);		// clears cache of a sentence

  void Insert(std::string &entries, long tID);
  void Execute(std::string command, long tID);
  void ExecuteDlt(std::map<std::string, std::string> dlt_meta, long tID);

protected:

  static PhraseDictionaryCache *s_instance;
  static std::map< const std::string, PhraseDictionaryCache * > s_instance_map;

  Scores Conv2VecFloats(std::string&);
  void Insert(std::vector<std::string> entries, long tID);

  void Update(long tID, std::vector<std::string> entries);
  void Update(long tID, std::string sourceString, std::string targetString, std::string ScoreString="", std::string waString="");
  void Update(long tID, Phrase p, TargetPhrase tp, Scores scores, std::string waString="");

  void Execute(std::vector<std::string> commands, long tID);
  void Execute_Single_Command(std::string command);


  void SetPreComputedScores(const unsigned int numScoreComponent);
  Scores GetPreComputedScores(const unsigned int age);

  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase) const;
};

}  // namespace Moses

#endif /* moses_PhraseDictionaryCache_H_ */
