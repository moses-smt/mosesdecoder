#include <stdexcept>
#include "InputFeature.h"
#include "moses/Util.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/InputPath.h"
#include "util/check.hh"

using namespace std;

namespace Moses
{
InputFeature::InputFeature(const std::string &line)
  :StatelessFeatureFunction("InputFeature", line)
{
  ReadParameters();
}

void InputFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "num-input-features") {
    m_numInputScores = Scan<size_t>(value);
  } else if (key == "real-word-count") {
    m_numRealWordCount = Scan<size_t>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }

}

void InputFeature::Evaluate(const InputType &input
                            , const InputPath &inputPath
                            , ScoreComponentCollection &scoreBreakdown) const
{
  const ScoreComponentCollection *scores = inputPath.GetInputScore();
  if (scores) {

  }
}

} // namespace

