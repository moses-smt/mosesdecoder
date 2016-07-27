#include "moses/FF/FFState.h"
#include "moses/Util.h"
#include "moses/PipelinedLM.h"
#include "moses/HypothesisStackCubePruningPipelined.h"

using namespace std;

namespace Moses
{

void PipelinedLM::EvaluateWhenApplied(Hypothesis &hypo) 
{
  const Hypothesis* prevHypo = hypo.GetPrevHypo();
  FFState const* ps = prevHypo ? prevHypo->GetFFState(GetStateIndex()) : NULL;
  const lm::ngram::State &in_state = static_cast<const KenLMState&>(*ps).state;
  KenLMState* out_state = new KenLMState();

  if (!hypo.GetCurrTargetLength()) {
    //simply set the in_state as the out_state for the hypothesis FFStates
    out_state->state = in_state;
    hypo.SetFFState(GetStateIndex(), out_state);
    m_stack->AddScored(&hypo);
    return;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_lm.GetModel().Order() - 1);
  std::size_t position = begin;
  bool pipeline_should_set_state = true;

  if (hypo.IsSourceCompleted()) {
    // Score end of sentence.
    pipeline_should_set_state = false;
    std::vector<lm::WordIndex> indices(m_lm.GetModel().Order() - 1);
    const lm::WordIndex *last = m_lm.LastIDs(hypo, &indices.front());
    float eos_score = m_lm.GetModel().FullScoreForgotState(&indices.front(), last, m_lm.GetModel().GetVocabulary().EndSentence(), out_state->state).prob;
    hypo.AddResultOfPipelinedFeatureFunction(&m_lm, TransformLMScore(eos_score));
  } else if (adjust_end < end) {
    // Get state after adding a long phrase and write it to out_state
    pipeline_should_set_state = false;
    std::vector<lm::WordIndex> indices(m_lm.GetModel().Order() - 1);
    const lm::WordIndex *last = m_lm.LastIDs(hypo, &indices.front());
    m_lm.GetModel().GetState(&indices.front(), last, out_state->state);
  }

  typename PipelinedCallback::Argument arg = {&hypo, out_state, pipeline_should_set_state};
  m_pipeline.BeginScore(in_state, m_lm.TranslateID(hypo.GetWord(position)), arg);
  ++position;
  for (; position < adjust_end; ++position) {
    m_pipeline.AppendWord(m_lm.TranslateID(hypo.GetWord(position)));
  }
}

void PipelinedCallback::operator()(lm::ngram::State state, lm::FullScoreReturn score, Argument arg)
{
  //set LM state only if it was not already set (it is set when eos is reached or when a long phrase was added)
  if (arg.pipeline_should_set_state) {
    arg.out_state->state = state;
  }
  arg.hypo->SetFFState(pipelinedLM.GetStateIndex(), arg.out_state); 
  arg.hypo->AddResultOfPipelinedFeatureFunction(&pipelinedLM.GetLM(), TransformLMScore(score.prob));
  pipelinedLM.GetStack()->AddScored(arg.hypo);
}
}
