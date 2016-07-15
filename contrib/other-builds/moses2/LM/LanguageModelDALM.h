/*
 * LanguageModelDALM.h
 *
 *  Created on: 5 Dec 2015
 *      Author: hieu
 */

#pragma once
#include "../FF/StatefulFeatureFunction.h"
#include "../legacy/Util2.h"
#include "../legacy/Factor.h"

namespace DALM
{
class Logger;
class Vocabulary;
class State;
class LM;
union Fragment;
class Gap;

typedef unsigned int VocabId;
}

namespace Moses2
{

class LanguageModelDALM: public StatefulFeatureFunction
{
public:
  LanguageModelDALM(size_t startInd, const std::string &line);
  virtual ~LanguageModelDALM();

  virtual void Load(System &system);
  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual FFState* BlankState(MemPool &pool, const System &sys) const;
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
      const InputType &input, const Hypothesis &hypo) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase &source,
      const TargetPhraseImpl &targetPhrase, Scores &scores,
      SCORE &estimatedScore) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
      const Hypothesis &hypo, const FFState &prevState, Scores &scores,
      FFState &state) const;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
      const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
      FFState &state) const;

protected:
  FactorType m_factorType;

  std::string m_filePath;
  size_t m_nGramOrder; //! max n-gram length contained in this LM
  size_t m_ContextSize;

  DALM::Logger *m_logger;
  DALM::Vocabulary *m_vocab;
  DALM::LM *m_lm;
  DALM::VocabId wid_start, wid_end;

  const Factor *m_beginSentenceFactor;

  mutable std::vector<DALM::VocabId> m_vocabMap;

  void CreateVocabMapping(const std::string &wordstxt, const System &system);

};

}

