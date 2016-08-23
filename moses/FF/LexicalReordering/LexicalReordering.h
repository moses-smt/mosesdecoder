// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <string>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "moses/Factor.h"
#include "moses/Phrase.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Range.h"
#include "moses/TranslationOption.h"

#include "moses/FF/StatefulFeatureFunction.h"
#include "util/exception.hh"

#include "LRState.h"
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
  void Load(AllOptions::ptr const& opts);

  virtual
  bool
  IsUseable(const FactorMask &mask) const;

  virtual
  FFState const*
  EmptyHypothesisState(const InputType &input) const;

  void
  InitializeForInput(ttasksptr const& ttask) {
    if (m_table) m_table->InitializeForInput(ttask);
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



