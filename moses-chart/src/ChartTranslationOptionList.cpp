
#include <algorithm>
#include <iostream>
#include "ChartTranslationOptionList.h"
#include "../../moses/src/Util.h"

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

