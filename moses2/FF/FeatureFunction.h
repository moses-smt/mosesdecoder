/*
 * FeatureFunction.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "../TypeDef.h"
#include "../Phrase.h"

namespace Moses2
{
template<typename WORD>
class TargetPhrase;

class System;
class PhraseImpl;
class TargetPhrases;
class TargetPhraseImpl;
class Scores;
class ManagerBase;
class MemPool;

namespace SCFG
{
class TargetPhrase;
class TargetPhrases;
class Word;
}

class FeatureFunction
{
public:

  FeatureFunction(size_t startInd, const std::string &line);
  virtual ~FeatureFunction();
  virtual void Load(System &system) {
  }

  size_t GetStartInd() const {
    return m_startInd;
  }
  size_t GetNumScores() const {
    return m_numScores;
  }
  const std::string &GetName() const {
    return m_name;
  }
  void SetName(const std::string &val) {
    m_name = val;
  }

  virtual size_t HasPhraseTableInd() const {
    return false;
  }
  void SetPhraseTableInd(size_t ind) {
    m_PhraseTableInd = ind;
  }
  size_t GetPhraseTableInd() const {
    return m_PhraseTableInd;
  }

  //! if false, then this feature is not displayed in the n-best list.
  // use with care
  virtual bool IsTuneable() const {
    return m_tuneable;
  }

  virtual void SetParameter(const std::string& key, const std::string& value);

  // may have more factors than actually need, but not guaranteed.
  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
                      const TargetPhraseImpl &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const = 0;

  // For SCFG decoding, the source can contain non-terminals, NOT the raw
  // source from the input sentence
  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const = 0;

  // used by lexicalised reordering model to add scores to tp data structures
  virtual void EvaluateAfterTablePruning(MemPool &pool,
                                         const TargetPhrases &tps, const Phrase<Moses2::Word> &sourcePhrase) const {
  }

  virtual void EvaluateAfterTablePruning(MemPool &pool,
                                         const SCFG::TargetPhrases &tps, const Phrase<SCFG::Word> &sourcePhrase) const {
  }

  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing() const {
  }

protected:
  size_t m_startInd;
  size_t m_numScores;
  size_t m_PhraseTableInd;
  std::string m_name;
  std::vector<std::vector<std::string> > m_args;
  bool m_tuneable;

  virtual void ReadParameters();
  void ParseLine(const std::string &line);
};

}

