// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"
#include <boost/thread/tss.hpp>
#include "lm/model.hh"
#include "moses/LM/Ken.h"
#include "moses/FF/FFState.h"

namespace Moses
{

struct InMemoryPerSentenceOnDemandLMState : public FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const InMemoryPerSentenceOnDemandLMState &other = static_cast<const InMemoryPerSentenceOnDemandLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }

};

class InMemoryPerSentenceOnDemandLM : public LanguageModel
{
public:
  InMemoryPerSentenceOnDemandLM(const std::string &line);
  ~InMemoryPerSentenceOnDemandLM();

  void InitializeForInput(ttasksptr const& ttask);

  virtual void SetParameter(const std::string& key, const std::string& value) {
    GetPerThreadLM().SetParameter(key, value);
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    if (initialized) {
      return GetPerThreadLM().EmptyHypothesisState(input);
    } else {
      return new InMemoryPerSentenceOnDemandLMState();
    }
  }

  virtual FFState *EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const {
    if (initialized) {
      return GetPerThreadLM().EvaluateWhenApplied(hypo, ps, out);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }

  virtual FFState *EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection *accumulator) const {
    if (initialized) {
      return GetPerThreadLM().EvaluateWhenApplied(cur_hypo, featureID, accumulator);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }

  virtual FFState *EvaluateWhenApplied(const Syntax::SHyperedge& hyperedge, int featureID, ScoreComponentCollection *accumulator) const {
    if (initialized) {
      return GetPerThreadLM().EvaluateWhenApplied(hyperedge, featureID, accumulator);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }


  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const {
    if (initialized) {
      GetPerThreadLM().CalcScore(phrase, fullScore, ngramScore, oovCount);
    }
  }

  virtual void CalcScoreFromCache(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const {
    if (initialized) {
      GetPerThreadLM().CalcScoreFromCache(phrase, fullScore, ngramScore, oovCount);
    }
  }

  virtual void IssueRequestsFor(Hypothesis& hypo, const FFState* input_state) {
    GetPerThreadLM().IssueRequestsFor(hypo, input_state);
  }

  virtual void sync() {
    GetPerThreadLM().sync();
  }

  virtual void SetFFStateIdx(int state_idx) {
    if (initialized) {
      GetPerThreadLM().SetFFStateIdx(state_idx);
    }
  }

  virtual void IncrementalCallback(Incremental::Manager &manager) const {
    if (initialized) {
      GetPerThreadLM().IncrementalCallback(manager);
    }
  }

  virtual void ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const {
    if (initialized) {
      GetPerThreadLM().ReportHistoryOrder(out, phrase);
    }
  }

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedScores) const {
    if (initialized) {
      GetPerThreadLM().EvaluateInIsolation(source, targetPhrase, scoreBreakdown, estimatedScores);
    }
  }

  bool IsUseable(const FactorMask &mask) const {
    return GetPerThreadLM().IsUseable(mask);
  }


protected:
  LanguageModelKen<lm::ngram::ProbingModel> & GetPerThreadLM() const;

  mutable boost::thread_specific_ptr<LanguageModelKen<lm::ngram::ProbingModel> > m_perThreadLM;

  bool initialized;

};


}
