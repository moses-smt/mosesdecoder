
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

