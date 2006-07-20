
#include "ScoreComponentCollection.h"
#include "Dictionary.h"

void ScoreComponentCollection::Combine(const ScoreComponentCollection &otherComponentCollection)
{
	const_iterator iter;
	for (iter = otherComponentCollection.begin() ; iter != otherComponentCollection.end() ; iter++)
	{
		const ScoreComponent &newScoreComponent = iter->second;
		iterator iterThis = find(newScoreComponent.GetDictionary());
		assert (iterThis != end());
		
		// score component for dictionary exists. add numbers
		ScoreComponent &thisScoreComponent = iterThis->second;
		thisScoreComponent.Add(newScoreComponent);
	}
}