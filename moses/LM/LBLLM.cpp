#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include "LBLLM.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"

using namespace std;

namespace Moses
{
int LBLLMState::Compare(const FFState& other) const
{
  const LBLLMState &otherState = static_cast<const LBLLMState&>(other);

  if (m_targetLen == otherState.m_targetLen)
    return 0;
  return (m_targetLen < otherState.m_targetLen) ? -1 : +1;
}

////////////////////////////////////////////////////////////////
LBLLM::LBLLM(const std::string &line)
:StatefulFeatureFunction(3, line)
,lm(ModelData(), oxlm::Dict(), true)
{
  ReadParameters();
}

void LBLLM::Load()
{
	{
	  cerr << "Reading LM from " << m_lmPath << " ...\n";
	  //ifstream ifile(lm_file.c_str(), ios::in | ios::binary);
	  ifstream ifile(m_lmPath.c_str(), ios::in);
	  if (!ifile.good()) {
	    cerr << "Failed to open " << m_lmPath << " for reading\n";
	    abort();
	  }
	  boost::archive::text_iarchive ia(ifile);
	  ia >> lm;
	  dict = lm.label_set();
	}

}

void LBLLM::EvaluateInIsolation(const Phrase &source
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{}

void LBLLM::EvaluateWithSourceContext(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , const StackVec *stackVec
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{}

FFState* LBLLM::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  newScores[0] = 1.5;
  newScores[1] = 0.3;
  newScores[2] = 0.4;
  accumulator->PlusEquals(this, newScores);

  // sparse scores
  accumulator->PlusEquals(this, "sparse-name", 2.4);

  // int targetLen = cur_hypo.GetCurrTargetPhrase().GetSize(); // ??? [UG]
  return new LBLLMState(0);
}

FFState* LBLLM::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new LBLLMState(0);
}

void LBLLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "lm-file") {
	  m_lmPath = value;
  }
  else if (key == "ref-file") {
	  m_refPath = value;
  }
  else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

}

