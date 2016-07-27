#ifndef moses_PipelinedLM_h
#define moses_PipelinedLM_h

#include "FF/StatefulFeatureFunction.h"
#include "LM/Ken.h"
#include "lm/model.hh"
#include "lm/automaton.hh"
#include "moses/FF/FFState.h"
#include "moses/StaticData.h"
namespace Moses
{

class HypothesisStackCubePruningPipelined;
class PipelinedLM;

struct PipelinedCallback {
  PipelinedLM& pipelinedLM;

  PipelinedCallback(PipelinedLM& p) : pipelinedLM(p) {}

  struct Argument {
    Hypothesis* hypo;
    KenLMState* out_state;
    bool pipeline_should_set_state; 
  };
  //this will be the callback called when a hypothesis is scored
  void operator()(lm::ngram::State state, lm::FullScoreReturn score, Argument arg);
};

class PipelinedLM  
{
  typedef typename lm::ngram::NGramAutomaton<lm::ngram::BackoffValue, PipelinedCallback>::Construct ConstructT;
  typedef LanguageModelKen<lm::ngram::ProbingModel> LM; 

  public:
  PipelinedLM(LM& lm) :
    m_lm(lm),
    m_pipeline(StaticData::Instance().options()->cube.pipeline_size, ConstructT(m_lm.GetModel().GetSearch(), PipelinedCallback(*this))),
    m_stack(NULL) {}

  // Needs to be called before the pipelined LM is used
  // and then every time a different stack is being filled
  void SetStack(HypothesisStackCubePruningPipelined* stack) {
    m_stack = stack;
  }

  HypothesisStackCubePruningPipelined* GetStack() { 
    return m_stack;
  }

  const LM& GetLM(){
    return m_lm;
  }

  void EvaluateWhenApplied(Hypothesis&);
  void Drain() {m_pipeline.Drain();}
  std::size_t GetStateIndex() {return m_lm.GetStateIndex();}



  private:
  LM& m_lm;
  lm::Pipeline<PipelinedCallback> m_pipeline;
  HypothesisStackCubePruningPipelined* m_stack;
};
} //namespace Moses
#endif
