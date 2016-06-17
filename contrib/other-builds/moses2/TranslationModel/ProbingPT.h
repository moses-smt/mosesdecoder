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

namespace Moses2
{

class QueryEngine;
class target_text;
class MemPool;
class System;
class RecycleData;

class ProbingPT: public PhraseTable
{
  //////////////////////////////////////
    class ActiveChartEntryProbing : public SCFG::ActiveChartEntry
    {
      typedef SCFG::ActiveChartEntry Parent;
    public:

      ActiveChartEntryProbing(MemPool &pool)
      :Parent(pool)
      ,m_hash(0)
      {}

      ActiveChartEntryProbing(
          MemPool &pool,
          const ActiveChartEntry &prevEntry)
      :Parent(prevEntry)
      ,m_hash(0)
      {}

      uint64_t GetHash() const
      { return m_hash; }

      uint64_t GetHash(const SCFG::Word &nextWord, const ProbingPT &pt) const;

      virtual void AddSymbolBindElement(
          const Range &range,
          const SCFG::Word &word,
          const Moses2::HypothesisColl *hypos,
          const PhraseTable &pt);

    protected:
      uint64_t m_hash;
    };
    //////////////////////////////////////

public:
  ProbingPT(size_t startInd, const std::string &line);
  virtual ~ProbingPT();
  void Load(System &system);

  void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;

  void InitActiveChart(MemPool &pool, SCFG::InputPath &path) const;

  virtual void Lookup(MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

  void LookupUnary(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

  uint64_t GetUnk() const
  { m_unkId; }
protected:
  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*

  uint64_t m_unkId;
  QueryEngine *m_engine;

  boost::iostreams::mapped_file_source file;
  const char *data;

  TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;
  TargetPhrases *CreateTargetPhrase(MemPool &pool, const System &system,
      const Phrase<Moses2::Word> &sourcePhrase, uint64_t key) const;
  TargetPhrase<Moses2::Word> *CreateTargetPhrase(MemPool &pool, const System &system,
      const char *&offset) const;

  inline const Factor *GetTargetFactor(uint32_t probingId) const
  {
    if (probingId >= m_targetVocab.size()) {
      return NULL;
    }
    return m_targetVocab[probingId];
  }

  std::pair<bool, uint64_t> GetHash(const Phrase<Moses2::Word> &sourcePhrase) const;

  void GetSourceProbingIds(const Phrase<Moses2::Word> &sourcePhrase, bool &ok,
      uint64_t probingSource[]) const;

  uint64_t GetSourceProbingId(const Word &word) const;

  // caching
  typedef boost::unordered_map<uint64_t, TargetPhrases*> Cache;
  Cache m_cache;

  void CreateCache(System &system);

  void ReformatWord(System &system, std::string &wordStr, bool &isNT);

  // scfg
  void LookupNT(
      MemPool &pool,
      const Moses2::Range &subPhraseRange,
      const SCFG::InputPath &prevPath,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &outPath) const;

  void LookupGivenWord(
      MemPool &pool,
      const SCFG::InputPath &prevPath,
      const SCFG::Word &wordSought,
      const Moses2::HypothesisColl *hypos,
      const Moses2::Range &subPhraseRange,
      SCFG::InputPath &outPath) const;

  void LookupGivenNode(
      MemPool &pool,
      const ActiveChartEntryProbing &prevEntry,
      const SCFG::Word &wordSought,
      const Moses2::HypothesisColl *hypos,
      const Moses2::Range &subPhraseRange,
      SCFG::InputPath &outPath) const;

};

}

