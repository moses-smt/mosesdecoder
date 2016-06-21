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
class InputPath;
class Manager;
class TargetPhrases;

namespace SCFG
{
class InputPath;
class Stacks;
class Manager;
}

////////////////////////////////////////////////////////////////////////
class PhraseTable: public StatelessFeatureFunction
{
public:
  int decodeGraphBackoff;

  PhraseTable(size_t startInd, const std::string &line);
  virtual ~PhraseTable();

  virtual void SetParameter(const std::string& key, const std::string& value);
  virtual void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;
  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;

  void SetPtInd(size_t ind)
  {  m_ptInd = ind; }

  size_t GetPtInd() const
  {  return m_ptInd; }

  bool SatisfyBackoff(const Manager &mgr, const InputPath &path) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
      const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void CleanUpAfterSentenceProcessing();

  // scfg
  virtual void InitActiveChart(
      MemPool &pool,
      const SCFG::Manager &mgr,
      SCFG::InputPath &path) const;

  virtual void Lookup(
      MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const = 0;

  virtual void LookupUnary(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const = 0;

protected:
  std::string m_path;
  size_t m_ptInd; // in the order that it is list in [feature], NOT order of [mapping]
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

