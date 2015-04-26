// -*- c++ -*-
#pragma once

#include <string>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "moses/Factor.h"
#include "moses/Phrase.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/WordsRange.h"
#include "moses/TranslationOption.h"

#include "moses/FF/StatefulFeatureFunction.h"
#include "util/exception.hh"

#include "LexicalReorderingState.h"
#include "LexicalReorderingTable.h"
#include "SparseReordering.h"


namespace Moses
{
class Factor;
class Phrase;
class Hypothesis;
class InputType;

// implementation of lexical reordering (Tilman ...) for phrase-based
// decoding
class LexicalReordering : public StatefulFeatureFunction
{
public:
  LexicalReordering(const std::string &line);
  virtual ~LexicalReordering();
  void Load();

  virtual
  bool
  IsUseable(const FactorMask &mask) const;

  virtual
  FFState const*
  EmptyHypothesisState(const InputType &input) const;

  void
  InitializeForInput(const InputType& i) {
    if (m_table) m_table->InitializeForInput(i);
  }

  Scores
  GetProb(const Phrase& f, const Phrase& e) const;

  virtual
  FFState*
  EvaluateWhenApplied(const Hypothesis& cur_hypo,
                      const FFState* prev_state,
                      ScoreComponentCollection* accumulator) const;

  virtual
  FFState*
  EvaluateWhenApplied(const ChartHypothesis&, int featureID,
                      ScoreComponentCollection*) const {
    UTIL_THROW2("LexicalReordering is not valid for chart decoder");
  }

  void
  EvaluateWithSourceContext
  (const InputType &input,
   const InputPath &inputPath,
   const TargetPhrase &targetPhrase,
   const StackVec *stackVec,
   ScoreComponentCollection& scoreBreakdown,
   ScoreComponentCollection* estimatedFutureScore = NULL) const
  { }

  void
  EvaluateTranslationOptionListWithSourceContext
  (const InputType &input, const TranslationOptionList &transOptList) const
  { }

  void
  EvaluateInIsolation(const Phrase &source,
                      const TargetPhrase &targetPhrase,
                      ScoreComponentCollection &scoreBreakdown,
                      ScoreComponentCollection &estimatedFutureScore) const
  { }

  bool
  GetHaveDefaultScores() {
    return m_haveDefaultScores;
  }

  float
  GetDefaultScore( size_t i ) {
    return m_defaultScores[i];
  }

  virtual
  void
  SetCache(TranslationOption& to) const;

  virtual
  void
  SetCache(TranslationOptionList& tol) const;

private:
  bool DecodeCondition(std::string s);
  bool DecodeDirection(std::string s);
  bool DecodeNumFeatureFunctions(std::string s);

  boost::scoped_ptr<LRModel> m_configuration;
  std::string m_modelTypeString;
  std::vector<std::string> m_modelType;
  boost::scoped_ptr<LexicalReorderingTable> m_table;
  std::vector<LRModel::Condition> m_condition;
  std::vector<FactorType> m_factorsE, m_factorsF;
  std::string m_filePath;
  bool m_haveDefaultScores;
  Scores m_defaultScores;
public:
  LRModel const& GetModel() const;
};

}



