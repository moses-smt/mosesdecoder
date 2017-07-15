/*
 * FeatureFunctions.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <boost/unordered_map.hpp>
#include <vector>
#include <string>
#include "../legacy/Parameter.h"
#include "../Phrase.h"

namespace Moses2
{
template<typename WORD>
class TargetPhrase;

class System;
class FeatureFunction;
class StatefulFeatureFunction;
class PhraseTable;
class Manager;
class MemPool;
class PhraseImpl;
class TargetPhrases;
class TargetPhraseImpl;
class Scores;
class Hypothesis;
class UnknownWordPenalty;
class Weights;

namespace SCFG
{
class TargetPhraseImpl;
class TargetPhrases;
class Word;
}

class FeatureFunctions
{
public:
  std::vector<const PhraseTable*> phraseTables;

  FeatureFunctions(System &system);
  virtual ~FeatureFunctions();

  const std::vector<const FeatureFunction*> &GetFeatureFunctions() const {
    return m_featureFunctions;
  }

  const std::vector<const StatefulFeatureFunction*> &GetStatefulFeatureFunctions() const {
    return m_statefulFeatureFunctions;
  }

  const std::vector<const FeatureFunction*> &GetWithPhraseTableInd() const {
    return m_withPhraseTableInd;
  }

  size_t GetNumScores() const {
    return m_ffStartInd;
  }

  void Create();
  void Load();

  const FeatureFunction *FindFeatureFunction(const std::string &name) const;

  const PhraseTable *GetPhraseTableExcludeUnknownWordPenalty(size_t ptInd);
  const UnknownWordPenalty *GetUnknownWordPenalty() const {
    return m_unkWP;
  }

  // the pool here must be the system pool if the rule was loaded during load, or the mgr pool if it was loaded on demand
  void EvaluateInIsolation(MemPool &pool, const System &system,
                           const Phrase<Moses2::Word> &source, TargetPhraseImpl &targetPhrase) const;
  void EvaluateInIsolation(MemPool &pool, const System &system,
                           const Phrase<SCFG::Word> &source, SCFG::TargetPhraseImpl &targetPhrase) const;

  void EvaluateAfterTablePruning(MemPool &pool, const TargetPhrases &tps,
                                 const Phrase<Moses2::Word> &sourcePhrase) const;
  void EvaluateAfterTablePruning(MemPool &pool, const SCFG::TargetPhrases &tps,
                                 const Phrase<SCFG::Word> &sourcePhrase) const;

  void EvaluateWhenAppliedBatch(const Batch &batch) const;

  void CleanUpAfterSentenceProcessing() const;

  void ShowWeights(const Weights &allWeights);

protected:
  std::vector<const FeatureFunction*> m_featureFunctions;
  std::vector<const StatefulFeatureFunction*> m_statefulFeatureFunctions;
  std::vector<const FeatureFunction*> m_withPhraseTableInd;
  const UnknownWordPenalty *m_unkWP;

  boost::unordered_map<std::string, size_t> m_defaultNames;
  System &m_system;
  size_t m_ffStartInd;

  FeatureFunction *Create(const std::string &line);
  std::string GetDefaultName(const std::string &stub);
  void OverrideFeatures();
  FeatureFunction *FindFeatureFunction(const std::string &name);

};

}

