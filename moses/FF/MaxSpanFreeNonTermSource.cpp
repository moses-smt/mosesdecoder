#include <iostream>
#include "MaxSpanFreeNonTermSource.h"
#include "moses/StaticData.h"
#include "moses/Word.h"
#include "moses/InputPath.h"

using namespace std;

namespace Moses
{
MaxSpanFreeNonTermSource::MaxSpanFreeNonTermSource(const std::string &line)
:StatelessFeatureFunction(1, line)
,m_maxSpan(true)
{
  m_tuneable = false;
  ReadParameters();
}

void MaxSpanFreeNonTermSource::Evaluate(const Phrase &source
						, const TargetPhrase &targetPhrase
						, ScoreComponentCollection &scoreBreakdown
						, ScoreComponentCollection &estimatedFutureScore) const
{
  float score = 0;


  scoreBreakdown.PlusEquals(this, score);
}

void MaxSpanFreeNonTermSource::Evaluate(const InputType &input
                       , const InputPath &inputPath
                       , const TargetPhrase &targetPhrase
                       , ScoreComponentCollection &scoreBreakdown
                       , ScoreComponentCollection *estimatedFutureScore) const
{
  cerr << "inputPath=" << inputPath.GetPhrase() << endl;
}


void MaxSpanFreeNonTermSource::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "max-span") {
	  m_maxSpan = Scan<int>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

std::vector<float> MaxSpanFreeNonTermSource::DefaultWeights() const
{
	std::vector<float> ret(1, 1);
	return ret;
}

}

