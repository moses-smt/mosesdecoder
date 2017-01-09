// $Id$
#pragma once

#include <string>
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
    if (isInitialized()) {
      return GetPerThreadLM().EmptyHypothesisState(input);
    } else {
      return new InMemoryPerSentenceOnDemandLMState();
    }
  }

  virtual FFState *EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const {
    if (isInitialized()) {
      return GetPerThreadLM().EvaluateWhenApplied(hypo, ps, out);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }

  virtual FFState *EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureID, ScoreComponentCollection *accumulator) const {
    if (isInitialized()) {
      return GetPerThreadLM().EvaluateWhenApplied(cur_hypo, featureID, accumulator);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }

  virtual FFState *EvaluateWhenApplied(const Syntax::SHyperedge& hyperedge, int featureID, ScoreComponentCollection *accumulator) const {
    if (isInitialized()) {
      return GetPerThreadLM().EvaluateWhenApplied(hyperedge, featureID, accumulator);
    } else {
      UTIL_THROW(util::Exception, "Can't evaluate an uninitialized LM\n");
    }
  }


  virtual void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const {
    if (isInitialized()) {
      GetPerThreadLM().CalcScore(phrase, fullScore, ngramScore, oovCount);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::CalcScore called prior to being initialized");
    }
  }

  virtual void CalcScoreFromCache(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const {
    if (isInitialized()) {
      GetPerThreadLM().CalcScoreFromCache(phrase, fullScore, ngramScore, oovCount);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::CalcScoreFromCache called prior to being initialized");
    }
  }

  virtual void IssueRequestsFor(Hypothesis& hypo, const FFState* input_state) {
    if (isInitialized()) {
      GetPerThreadLM().IssueRequestsFor(hypo, input_state);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::IssueRequestsFor called prior to being initialized");
    }
  }

  virtual void sync() {
    if (isInitialized()) {
      GetPerThreadLM().sync();
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::sync called prior to being initialized");
    }
  }

  virtual void SetFFStateIdx(int state_idx) {
    if (isInitialized()) {
      GetPerThreadLM().SetFFStateIdx(state_idx);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::SetFFStateIdx called prior to being initialized");
    }
  }

  virtual void IncrementalCallback(Incremental::Manager &manager) const {
    if (isInitialized()) {
      GetPerThreadLM().IncrementalCallback(manager);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::IncrementalCallback called prior to being initialized");
    }
  }

  virtual void ReportHistoryOrder(std::ostream &out,const Phrase &phrase) const {
    if (isInitialized()) {
      GetPerThreadLM().ReportHistoryOrder(out, phrase);
    } else {
      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::ReportHistoryOrder called prior to being initialized");
    }
  }

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedScores) const {
    if (isInitialized()) {
      GetPerThreadLM().EvaluateInIsolation(source, targetPhrase, scoreBreakdown, estimatedScores);
    } else {
      //      UTIL_THROW(util::Exception, "WARNING: InMemoryPerSentenceOnDemand::EvaluateInIsolation called prior to being initialized");
    }
  }

  bool IsUseable(const FactorMask &mask) const {
    bool ret = mask[m_factorType];
    return ret;
  }


protected:
  LanguageModelKen<lm::ngram::ProbingModel> & GetPerThreadLM() const;

  mutable boost::thread_specific_ptr<LanguageModelKen<lm::ngram::ProbingModel> > m_perThreadLM;
  mutable boost::thread_specific_ptr<std::string> m_tmpFilename;

  FactorType m_factorType;

  bool isInitialized() const {
    if (m_tmpFilename.get() == NULL) {
      return false;
    } else {
      return true;
    }
  }

};


}
