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

#ifndef moses_PhraseDictionaryDynamicCacheBased_H
#define moses_PhraseDictionaryDynamicCacheBased_H

#include "moses/TypeDef.h"
#include "moses/TranslationModel/PhraseDictionary.h"

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

/** Implementation of a Cache-based phrase table.
 */
class PhraseDictionaryDynamicCacheBased : public PhraseDictionary
{

//  typedef std::vector<unsigned int> AgeCollection;
  typedef std::vector<unsigned int> AgeCollection;
  typedef boost::tuple<TargetPhraseCollection::shared_ptr , AgeCollection*, Scores*> TargetCollectionPair;
  typedef std::map<Phrase, TargetCollectionPair> cacheMap;

  // factored translation
  std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;

  // data structure for the cache
  cacheMap m_cacheTM;
  std::vector<Scores> precomputedScores;
  unsigned int m_maxAge;
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

  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryDynamicCacheBased&);

public:
  PhraseDictionaryDynamicCacheBased(const std::string &line);
  ~PhraseDictionaryDynamicCacheBased();

  inline const std::string GetName() {
    return m_name;
  };
  inline void SetName(const std::string name) {
    m_name = name;
  }

  static const PhraseDictionaryDynamicCacheBased* Instance(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }

  static PhraseDictionaryDynamicCacheBased* InstanceNonConst(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }


  static const PhraseDictionaryDynamicCacheBased& Instance() {
    return *s_instance;
  }

  static PhraseDictionaryDynamicCacheBased& InstanceNonConst() {
    return *s_instance;
  }

  void Load(AllOptions::ptr const& opts);
  void Load(const std::string files);

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection(const Phrase &src) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollectionLEGACY(Phrase const &src) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const;

  // for phrase-based model
  //  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  void SetParameter(const std::string& key, const std::string& value);

  void InitializeForInput(ttasksptr const& ttask);

  //  virtual void InitializeForInput(InputType const&) {
  //    /* Don't do anything source specific here as this object is shared between threads.*/
  //  }

  void Print() const;             // prints the cache
  void Clear();           // clears the cache

  void ClearEntries(std::string &entries);
  void ClearSource(std::string &entries);
  void Insert(std::string &entries);
  void Execute(std::string command);
  void ExecuteDlt(std::map<std::string, std::string> dlt_meta);

  void SetScoreType(size_t type);
  void SetMaxAge(unsigned int age);

protected:
  static PhraseDictionaryDynamicCacheBased *s_instance;
  static std::map< const std::string, PhraseDictionaryDynamicCacheBased * > s_instance_map;

  float decaying_score(const int age);  // calculates the decay score given the age
  Scores Conv2VecFloats(std::string&);
  void Insert(std::vector<std::string> entries);

  void Decay();   // traverse through the cache and decay each entry
  void Decay(Phrase p);   // traverse through the cache and decay each entry for a given Phrase
  void Update(std::vector<std::string> entries, std::string ageString);
  void Update(std::string sourceString, std::string targetString, std::string ageString, std::string ScoreString="", std::string waString="");
  void Update(Phrase p, TargetPhrase tp, int age, Scores scores, std::string waString="");

  void ClearEntries(std::vector<std::string> entries);
  void ClearEntries(std::string sourceString, std::string targetString);
  void ClearEntries(Phrase p, Phrase tp);

  void ClearSource(std::vector<std::string> entries);
  void ClearSource(Phrase sp);

  void Execute(std::vector<std::string> commands);
  void Execute_Single_Command(std::string command);


  void SetPreComputedScores(const unsigned int numScoreComponent);
  Scores GetPreComputedScores(const unsigned int age);

  void Load_Multiple_Files(std::vector<std::string> files);
  void Load_Single_File(const std::string file);

  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase) const;
};

}  // namespace Moses

#endif /* moses_PhraseDictionaryDynamicCacheBased_H_ */
