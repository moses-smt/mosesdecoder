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
#include "../HypothesisColl.h"
#include "../FF/StatelessFeatureFunction.h"
#include "../legacy/Util2.h"

namespace Moses2
{

class System;
class InputPathsBase;
class InputPath;
class Manager;
class TargetPhrases;
class Range;

namespace SCFG
{
class InputPath;
class Stacks;
class Manager;
class ActiveChartEntry;
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

  void SetPtInd(size_t ind) {
    m_ptInd = ind;
  }

  size_t GetPtInd() const {
    return m_ptInd;
  }

  bool SatisfyBackoff(const Manager &mgr, const InputPath &path) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
                      const TargetPhraseImpl &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  // scfg
  virtual void InitActiveChart(
    MemPool &pool,
    const SCFG::Manager &mgr,
    SCFG::InputPath &path) const = 0;

  virtual void Lookup(
    MemPool &pool,
    const SCFG::Manager &mgr,
    size_t maxChartSpan,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const = 0;

  virtual void LookupUnary(MemPool &pool,
                           const SCFG::Manager &mgr,
                           const SCFG::Stacks &stacks,
                           SCFG::InputPath &path) const;

protected:
  std::string m_path;
  size_t m_ptInd; // in the order that it is list in [feature], NOT order of [mapping]
  size_t m_tableLimit;
  std::vector<FactorType> m_input, m_output;

  // cache
  size_t m_maxCacheSize; // 0 = no caching

  struct CacheCollEntry2 {
    TargetPhrases *tpsPtr;
    clock_t clock;
  };

  // scfg
  virtual void LookupNT(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const Moses2::Range &subPhraseRange,
    const SCFG::InputPath &prevPath,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &outPath) const;

  virtual void LookupGivenWord(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::InputPath &prevPath,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const;

  virtual void LookupGivenNode(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::ActiveChartEntry &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const = 0;

};

}

