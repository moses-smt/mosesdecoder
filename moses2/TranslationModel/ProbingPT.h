/*
 * ProbingPT.h
 *
 *  Created on: 3 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/thread/tss.hpp>
#include <boost/bimap.hpp>
#include <deque>
#include "PhraseTable.h"
#include "../Vector.h"
#include "../Phrase.h"
#include "../SCFG/ActiveChart.h"
#include "util/mmap.hh"

namespace probingpt
{
class QueryEngine;
class target_text;
}

namespace Moses2
{
class AlignmentInfo;
class MemPool;
class System;
class RecycleData;

namespace SCFG
{
class TargetPhraseImpl;
class TargetPhrases;
}

class ProbingPT: public Moses2::PhraseTable
{
  //////////////////////////////////////
  class ActiveChartEntryProbing : public SCFG::ActiveChartEntry
  {
    typedef SCFG::ActiveChartEntry Parent;
  public:

    ActiveChartEntryProbing(MemPool &pool)
      :Parent(pool)
      ,m_key(0)
    {}

    ActiveChartEntryProbing(
      MemPool &pool,
      const ActiveChartEntryProbing &prevEntry);

    uint64_t GetKey() const {
      return m_key;
    }

    std::pair<bool, uint64_t> GetKey(const SCFG::Word &nextWord, const ProbingPT &pt) const;

    virtual void AddSymbolBindElement(
      const Range &range,
      const SCFG::Word &word,
      const Moses2::Hypotheses *hypos,
      const Moses2::PhraseTable &pt);

  protected:
    uint64_t m_key;
  };
  //////////////////////////////////////

public:
  ProbingPT(size_t startInd, const std::string &line);
  virtual ~ProbingPT();
  void Load(System &system);

  virtual void SetParameter(const std::string& key, const std::string& value);
  void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;

  uint64_t GetUnk() const {
    return m_unkId;
  }

  // SCFG
  void InitActiveChart(
    MemPool &pool,
    const SCFG::Manager &mgr,
    SCFG::InputPath &path) const;

  virtual void Lookup(MemPool &pool,
                      const SCFG::Manager &mgr,
                      size_t maxChartSpan,
                      const SCFG::Stacks &stacks,
                      SCFG::InputPath &path) const;


protected:
  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector< std::pair<bool, const Factor*> > m_targetVocab; // pt id -> factor*
  std::vector<const AlignmentInfo*> m_aligns;
  util::LoadMethod load_method;

  uint64_t m_unkId;
  probingpt::QueryEngine *m_engine;

  void CreateAlignmentMap(System &system, const std::string path);

  TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
                        InputPath &inputPath) const;
  TargetPhrases *CreateTargetPhrases(MemPool &pool, const System &system,
                                     const Phrase<Moses2::Word> &sourcePhrase, uint64_t key) const;
  TargetPhraseImpl *CreateTargetPhrase(MemPool &pool, const System &system,
                                       const char *&offset) const;

  inline const std::pair<bool, const Factor*> *GetTargetFactor(uint32_t probingId) const {
    if (probingId >= m_targetVocab.size()) {
      return NULL;
    }
    return &m_targetVocab[probingId];
  }

  std::pair<bool, uint64_t> GetKey(const Phrase<Moses2::Word> &sourcePhrase) const;

  void GetSourceProbingIds(const Phrase<Moses2::Word> &sourcePhrase, bool &ok,
                           uint64_t probingSource[]) const;

  uint64_t GetSourceProbingId(const Word &word) const;

  // caching
  typedef boost::unordered_map<uint64_t, TargetPhrases*> CachePb;
  CachePb m_cachePb;

  typedef boost::unordered_map<uint64_t, SCFG::TargetPhrases*> CacheSCFG;
  CacheSCFG m_cacheSCFG;

  void CreateCache(System &system);

  void ReformatWord(System &system, std::string &wordStr, bool &isNT);

  // SCFG
  void LookupGivenNode(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::ActiveChartEntry &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const;

  std::pair<bool, SCFG::TargetPhrases*> CreateTargetPhrasesSCFG(MemPool &pool, const System &system,
      const Phrase<SCFG::Word> &sourcePhrase, uint64_t key) const;
  // return value: 1st = there are actual rules, not just a empty cell for prefix

  SCFG::TargetPhraseImpl *CreateTargetPhraseSCFG(
    MemPool &pool,
    const System &system,
    const char *&offset) const;


};

}

