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
  PipeLMState* out_state = new PipeLMState();

  if (!hypo.GetCurrTargetLength()) {
    //no work to be done
    //simply set the in_state as the out_state for the hypothesis FFStates
    out_state->state = in_state;
    SetLMState(hypo, out_state);
    m_stack->AddScored(&hypo);
    return;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_lm.GetModel().Order() - 1);
  std::size_t position = begin;
  std::size_t num_words = adjust_end - begin;
  out_state->callbacks_remaining = num_words;
  out_state->score = 0.0f;

  //Decide who should set the final out state
  bool pipeline_should_set_state = true;
  if (hypo.IsSourceCompleted()) {
    // Score end of sentence.
    // TODO: Decide whether this should be pipelined too!
    pipeline_should_set_state = false;
    std::vector<lm::WordIndex> indices(m_lm.GetModel().Order() - 1);
    const lm::WordIndex *last = m_lm.LastIDs(hypo, &indices.front());
    out_state->score += m_lm.GetModel().FullScoreForgotState(&indices.front(), last, m_lm.GetModel().GetVocabulary().EndSentence(), out_state->state).prob;
  } else if (adjust_end < end) {
    // Get state after adding a long phrase and write it to out_state
    pipeline_should_set_state = false;
    std::vector<lm::WordIndex> indices(m_lm.GetModel().Order() - 1);
    const lm::WordIndex *last = m_lm.LastIDs(hypo, &indices.front());
    m_lm.GetModel().GetState(&indices.front(), last, out_state->state);
  }


  EvaluateLM callback = {&hypo, m_stack, this, out_state, false};

  if (num_words == 1) {
    callback.m_should_set_ffstate = pipeline_should_set_state;
    m_pipeline.BeginScore(in_state, m_lm.TranslateID(hypo.GetWord(position)), callback);
  } else {
    m_pipeline.BeginScore(in_state, m_lm.TranslateID(hypo.GetWord(position)), callback);
    ++position;
    for (; position < adjust_end-1; ++position) {
      m_pipeline.AppendWord(m_lm.TranslateID(hypo.GetWord(position)), callback);
    }
    callback.m_should_set_ffstate = pipeline_should_set_state;
    m_pipeline.AppendWord(m_lm.TranslateID(hypo.GetWord(position)), callback);
  }
}

void PipelinedLM::SetLMState(Hypothesis& hypo, PipeLMState* state) {
  hypo.SetFFState(GetStateIndex(), state);
}

void PipelinedLM::AddLMScore(Hypothesis& hypo, float score) {
  score = TransformLMScore(score);
  hypo.AddResultOfPipelinedFeatureFunction(&m_lm, score);
}

void EvaluateLM::operator()(lm::FullScoreReturn r, lm::ngram::State state){
  m_state->score += r.prob;

  if (m_should_set_ffstate) {
    m_state->state = state;
  }
  if (--(m_state->callbacks_remaining) == 0) {
    m_pipelined_lm->SetLMState(*m_hypo, m_state);
    m_pipelined_lm->AddLMScore(*m_hypo, m_state->score);
    m_stack->AddScored(m_hypo); //TODO:
  }
}
}
