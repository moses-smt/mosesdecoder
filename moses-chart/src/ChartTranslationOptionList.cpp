
#include <algorithm>
#include <iostream>
#include "ChartTranslationOptionList.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/StaticData.h"

using namespace Moses;
using namespace std;

namespace MosesChart
{

TranslationOptionList::~TranslationOptionList()
{
	RemoveAllInColl(m_coll);
}

void TranslationOptionList::Add(TranslationOption *transOpt)
{
	m_coll.push_back(transOpt);
}

void TranslationOptionList::Sort()
{
	// keep only those over best + threshold
	
	float scoreThreshold = -std::numeric_limits<float>::infinity();
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const TranslationOption *transOpt = *iter;
		float score = transOpt->GetTotalScore();
		scoreThreshold = (score > scoreThreshold) ? score : scoreThreshold;
	}
	
	scoreThreshold += StaticData::Instance().GetTranslationOptionThreshold();

	size_t ind = 0;
	while (ind < m_coll.size())
	{
		const TranslationOption *transOpt = m_coll[ind];
		if (transOpt->GetTotalScore() < scoreThreshold)
		{
			delete transOpt;
			m_coll.erase(m_coll.begin() + ind);
		}
		else
		{
			ind++;
		}
	}

	std::sort(m_coll.begin(), m_coll.end(), ChartTranslationOptionOrderer());
}

void TranslationOptionList::DiscardDuplicates()
{
	typedef set<TranslationOption*, ChartTranslationOptionUnique> SetType;
	SetType uniqueTransOpt;
	
	// add them to set
	CollType::const_iterator iterVec;
	for (iterVec = m_coll.begin(); iterVec != m_coll.end(); ++iterVec)
	{
		TranslationOption *transOpt = *iterVec;
				
		// add 
		std::pair<SetType::iterator, bool> iterInsert = uniqueTransOpt.insert(transOpt);
		if (iterInsert.second)
		{ // added ok. do nothing
		}
		else
		{ // existing item. discard lowest score
			TranslationOption *otherTransOpt = *iterInsert.first;
			if (otherTransOpt->GetTotalScore() > transOpt->GetTotalScore())
			{ // keep item in set
				delete transOpt;
			}
			else
			{ // replace item in set with current trans opt
				delete otherTransOpt;
				uniqueTransOpt.erase(iterInsert.first);
			
				iterInsert = uniqueTransOpt.insert(transOpt);
				assert(iterInsert.second);
			}
		} // if (iterInsert.second)
	} // for (iterVec = m_coll.begin()
	
	// add unique items back into list
	m_coll.clear();
	std::copy(uniqueTransOpt.begin(), uniqueTransOpt.end(), std::inserter(m_coll, m_coll.end()));
	
	
}

std::ostream& operator<<(std::ostream &out, const TranslationOptionList &list)
{
	TranslationOptionList::const_iterator iter;
	for (iter = list.begin(); iter != list.end(); ++iter)
	{
		out << **iter << endl;
	}

	return out;
}

}

