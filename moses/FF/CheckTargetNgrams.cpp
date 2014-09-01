#include <vector>
#include "CheckTargetNgrams.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "BloomFilter.h"

using namespace std;

namespace Moses
{
int CheckTargetNgramsState::Compare(const FFState& other) const
{
  const CheckTargetNgramsState &otherState = static_cast<const CheckTargetNgramsState&>(other);

  if (m_history == otherState.m_history)
    return 0;
  return 1;
}

////////////////////////////////////////////////////////////////
CheckTargetNgrams::CheckTargetNgrams(const std::string &line)
  :StatefulFeatureFunction(3, line)
{
  ReadParameters();
  m_minorder=m_maxorder-m_numScoreComponents+1;
  UTIL_THROW_IF2(m_minorder<=0, "num-features is too high for max-order");  
}

void CheckTargetNgrams::EvaluateInIsolation(const Phrase &source
                                  , const TargetPhrase &targetPhrase
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection &estimatedFutureScore) const
{}

void CheckTargetNgrams::EvaluateWithSourceContext(const InputType &input
                                  , const InputPath &inputPath
                                  , const TargetPhrase &targetPhrase
                                  , const StackVec *stackVec
                                  , ScoreComponentCollection &scoreBreakdown
                                  , ScoreComponentCollection *estimatedFutureScore) const
{}

FFState* CheckTargetNgrams::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  // dense scores
  vector<float> newScores(m_numScoreComponents);
  for (size_t i=0; i<newScores.size(); ++i){
    newScores[i] = 1; //TODO: compute scores
  }

  accumulator->PlusEquals(this, newScores);

  // sparse scores
  //accumulator->PlusEquals(this, "sparse-name", 2.4);

  unsigned int history = 1; //TODO: compute hash function for state
  return new CheckTargetNgramsState(history);
}

FFState* CheckTargetNgrams::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  //this feature can be computed only when the LM is computed
  return new CheckTargetNgramsState(0);
}

void CheckTargetNgrams::SetParameter(const std::string& key, const std::string& value)
{
 if (key == "path") {
    m_filePath = value;
  } else if (key == "factor") {
    m_FactorsVec = Tokenize<FactorType>(value,",");
  } else if (key == "max-order") {
    m_maxorder = Tokenize<int>(value,",");
   //m_numScoreComponents is read and set in the super-class
   //if >1 then it will check n-grams from max-order to max-order - numScoreComponents +1
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


void CheckTargetNgrams::Load()
{
  if (m_filePath == "") 
    UTIL_THROW("Filename must be specified");
  ifstream inFile(m_filePath.c_str());
  UTIL_THROW_IF2(!inFile, "Can't open file " << m_filePath);
    
  //load bloom filter file
  m_bloom_filter.load(inFile);
  inFile.close();
}

}

