
#include "UnknownWordHandlerVerbatim.h"
#include "FactorCollection.h"
#include "Word.h"

using namespace std;

UnknownWordHandlerVerbatim::UnknownWordHandlerVerbatim(FactorType sourceFactorType, FactorType targetFactorType)
:UnknownWordHandler(sourceFactorType, targetFactorType)
{}

list<UnknownWordScorePair> UnknownWordHandlerVerbatim::GetUnknownWord(const Word &sourceWord) const
{
	FactorCollection &factorCollection = FactorCollection::Instance();

	list<UnknownWordScorePair> ret;

	const Factor *sourceFactor = sourceWord.GetFactor(m_sourceFactorType);
	const Factor *targetFactor;
	if (sourceFactor == NULL)
	{
		targetFactor  = factorCollection.AddFactor(Output, m_targetFactorType, UNKNOWN_FACTOR);
	}
	else
	{
		targetFactor = factorCollection.AddFactor(Output, m_targetFactorType, sourceFactor->GetString());
	}
	UnknownWordScorePair pair(targetFactor, -100);
	ret.push_back(pair);

	return ret;
}

