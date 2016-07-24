#ifndef moses_PipelinedLM_h
#define moses_PipelinedLM_h

#include "FF/StatefulFeatureFunction.h"
#include "LM/Ken.h"
#include "lm/model.hh"
#include "lm/automaton.hh"
#include "moses/FF/FFState.h"
namespace Moses
{

class PipelinedLM;
class HypothesisStackCubePruningPipelined;

struct PipeLMState : public KenLMState {
  float score;
  std::size_t callbacks_remaining;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const PipeLMState &other = static_cast<const PipeLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }
};

struct EvaluateLM {
  Hypothesis* m_hypo;
  HypothesisStackCubePruningPipelined* m_stack;
  PipelinedLM* m_pipelined_lm;
  PipeLMState* m_state;
  bool m_should_set_ffstate;

  void operator()(lm::FullScoreReturn r, lm::ngram::State state);
};

class PipelinedLM  
{
  typedef typename lm::ngram::NGramAutomaton<lm::ngram::BackoffValue, EvaluateLM>::Construct ConstructT;
  typedef LanguageModelKen<lm::ngram::ProbingModel> LM; 

  public:
  PipelinedLM(LM& lm, HypothesisStackCubePruningPipelined* stack, std::size_t state_index) :
    m_lm(lm),
    m_pipeline(16, ConstructT(m_lm.GetModel().GetSearch())),
    m_stack(stack),
    m_state_index(state_index) {}

  void EvaluateWhenApplied(Hypothesis&);
  void Drain() {m_pipeline.Drain();}
  std::size_t GetStateIndex() {return m_state_index;}

  void AddLMScore(Hypothesis& hypo, float score);
  void SetLMState(Hypothesis& hypo, PipeLMState* state);

  private:
  LM& m_lm;
  lm::Pipeline<EvaluateLM> m_pipeline;
  HypothesisStackCubePruningPipelined* m_stack;
  std::size_t m_state_index;
};
} //namespace Moses
#endif
