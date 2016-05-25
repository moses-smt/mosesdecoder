/*
 * UnknownWordPenalty.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "PhraseTable.h"

namespace Moses2
{

class UnknownWordPenalty: public PhraseTable
{
public:
  UnknownWordPenalty(size_t startInd, const std::string &line);
  virtual ~UnknownWordPenalty();

  void Lookup(const Manager &mgr, InputPathsBase &inputPaths) const;
  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;

  virtual void
  EvaluateInIsolation(const System &system, const Phrase<Moses2::Word> &source,
      const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void InitActiveChart(SCFG::InputPath &path) const;

  void Lookup(MemPool &pool,
      const SCFG::Manager &mgr,
      size_t maxChartSpan,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

  void LookupUnary(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

};

}

