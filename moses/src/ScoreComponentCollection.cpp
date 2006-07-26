
#include "ScoreComponentCollection.h"
#include "Dictionary.h"

using namespace std;

void ScoreComponentCollection::Combine(const ScoreComponentCollection &otherComponentCollection)
{
	const_iterator iter;
	for (iter = otherComponentCollection.begin() ; iter != otherComponentCollection.end() ; iter++)
	{
		const ScoreComponent &newScoreComponent = iter->second;
		iterator iterThis = m_scores.find(newScoreComponent.GetDictionary());
		assert (iterThis != m_scores.end());
		
		// score component for dictionary exists. add numbers
		ScoreComponent &thisScoreComponent = iterThis->second;
		thisScoreComponent.PlusEquals(newScoreComponent);
	}
}

// helper fns
bool CompareScoreComponent(const ScoreComponent* a, const ScoreComponent* b)
{
	return a->GetDictionary()->GetIndex() < b->GetDictionary()->GetIndex();
}

// sort by index of dictionaries
vector<const ScoreComponent*> ScoreComponentCollection::SortForNBestOutput() const
{
	vector<const ScoreComponent*> ret;
	for (const_iterator iter = m_scores.begin() ; iter != m_scores.end() ; ++iter)
		ret.push_back(&iter->second);
	
	sort(ret.begin(), ret.end(), CompareScoreComponent);
	return ret;
}

