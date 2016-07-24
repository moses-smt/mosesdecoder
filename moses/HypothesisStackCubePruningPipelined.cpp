#include "moses/HypothesisStackCubePruningPipelined.h"

using namespace std;

namespace Moses
{

namespace 
{
LanguageModelKen<lm::ngram::ProbingModel>& GetLM(const std::string& name) 
{
  FeatureFunction& ff = FeatureFunction::FindFeatureFunction(name);
  LanguageModelKen<lm::ngram::ProbingModel>& lm =
    static_cast<LanguageModelKen<lm::ngram::ProbingModel>& >(ff);
  return lm;
}

std::size_t GetStateIndex(const std::string& name) {
  const vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for(std::size_t i = 0; i < ffs.size(); ++i) 
    if (ffs[i]->GetScoreProducerDescription() == name) return i;
  UTIL_THROW2(name << " is not found");
}
}

HypothesisStackCubePruningPipelined::HypothesisStackCubePruningPipelined(Manager& manager) :
  HypothesisStackCubePruning(manager),
  m_lm0(GetLM("LM0"), this, GetStateIndex("LM0")),
  m_lm1(GetLM("LM1"), this, GetStateIndex("LM1")) {} 

bool HypothesisStackCubePruningPipelined::AddPrune(Hypothesis* hypo)
{
  m_lm0.EvaluateWhenApplied(*hypo);
  m_lm1.EvaluateWhenApplied(*hypo);
  //the return value is used to decide whether number of stack insertions in bitmapcontainer should be incremented or not the number of stack insertions only seems to be used for ensuring diversity
  //The final score for a hypothesis is computed as an inner product of weights and the computed ScoreCollection (which is a vector of scores) So once you compute the probability you should addPlus the score to the scoreComponentCollection stored in hypothesis and either recompute the whole inner product or extract the weight for LM (from static instance) and increment the m_futureScore of hypo by weight*score_computed float weight = StaticData::Instance().GetWeight(lm); Also you need to update the state in hypothesis
  return true;
}


void HypothesisStackCubePruningPipelined::AddScored(Hypothesis* hypo) {
  //once both pipelines have finished and set their states, proceed 
  if (hypo->GetFFState(m_lm0.GetStateIndex()) && hypo->GetFFState(m_lm1.GetStateIndex())) {
    HypothesisStackCubePruning::AddPrune(hypo);
  }
}

void HypothesisStackCubePruningPipelined::Drain() {
  m_lm0.Drain();
  m_lm1.Drain();
}
}
