/*
 * PhraseTable.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "../Word.h"
#include "../FF/StatelessFeatureFunction.h"
#include "../legacy/Util2.h"

namespace Moses2
{

class System;
class InputPathsBase;
class InputPathBase;
class Manager;
class TargetPhrases;

namespace SCFG
{
class InputPath;
class Stacks;
}

////////////////////////////////////////////////////////////////////////
class PhraseTable: public StatelessFeatureFunction
{
public:
  PhraseTable(size_t startInd, const std::string &line);
  virtual ~PhraseTable();

  virtual void SetParameter(const std::string& key, const std::string& value);
  virtual void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;
  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPathBase &inputPath) const;

  void SetPtInd(size_t ind)
  {
    m_ptInd = ind;
  }
  size_t GetPtInd() const
  {
    return m_ptInd;
  }

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase &source,
      const TargetPhrase &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void CleanUpAfterSentenceProcessing();

  // scfg
  virtual void InitActiveChart(SCFG::InputPath &path) const;
  virtual void Lookup(MemPool &pool,
      const System &system,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  std::string m_path;
  size_t m_ptInd;
  size_t m_tableLimit;

  // cache
  size_t m_maxCacheSize; // 0 = no caching

  struct CacheCollEntry2
  {
    TargetPhrases *tpsPtr;
    clock_t clock;
  };

};

}

