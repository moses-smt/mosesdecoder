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

namespace Moses2
{

class QueryEngine;
class target_text;
class MemPool;
class System;
class RecycleData;

class ProbingPT: public PhraseTable
{
public:
  ProbingPT(size_t startInd, const std::string &line);
  virtual ~ProbingPT();
  void Load(System &system);

  void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;

  virtual void Lookup(MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

  void LookupUnary(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*

  uint64_t m_unkId;
  QueryEngine *m_engine;

  boost::iostreams::mapped_file_source file;
  const char *data;

  mutable boost::thread_specific_ptr<std::deque<target_text*> > m_recycler;

  TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;
  TargetPhrases *CreateTargetPhrase(MemPool &pool, const System &system,
      const Phrase<Moses2::Word> &sourcePhrase, uint64_t key) const;
  TargetPhrase<Moses2::Word> *CreateTargetPhrase(MemPool &pool, const System &system,
      const char *&offset) const;

  void ConvertToProbingSourcePhrase(const Phrase<Moses2::Word> &sourcePhrase, bool &ok,
      uint64_t probingSource[]) const;

  inline const Factor *GetTargetFactor(uint32_t probingId) const
  {
    if (probingId >= m_targetVocab.size()) {
      return NULL;
    }
    return m_targetVocab[probingId];
  }

  std::pair<bool, uint64_t> GetSourceProbingId(
      const Phrase<Moses2::Word> &sourcePhrase) const;

  inline uint64_t GetSourceProbingId(const Factor *factor) const
  {
    size_t factorId = factor->GetId();
    if (factorId >= m_sourceVocab.size()) {
      return m_unkId;
    }
    return m_sourceVocab[factorId];

  }

  // caching
  typedef boost::unordered_map<uint64_t, TargetPhrases*> Cache;
  Cache m_cache;

  void CreateCache(System &system);

};

}

