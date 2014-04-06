#include "SetSourcePhrase.h"
#include "moses/TargetPhrase.h"

namespace Moses
{
SetSourcePhrase::SetSourcePhrase(const std::string &line)
:StatelessFeatureFunction(1, line)
{
	ReadParameters();
}

void SetSourcePhrase::Evaluate(const Phrase &source
						, const TargetPhrase &targetPhrase
						, ScoreComponentCollection &scoreBreakdown
						, ScoreComponentCollection &estimatedFutureScore) const
{
	targetPhrase.SetRuleSource(source);
}

}
