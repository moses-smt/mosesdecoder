
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOption.h"
#include "../../moses/src/Util.h"

using namespace Moses;

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

}

