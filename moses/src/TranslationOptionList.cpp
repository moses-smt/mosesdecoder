
#include "TranslationOptionList.h"
#include "Util.h"
#include "TranslationOption.h"

namespace Moses
{

TranslationOptionList::TranslationOptionList(const TranslationOptionList &copy)
{
	const_iterator iter;
	for (iter = copy.begin(); iter != copy.end(); ++iter)
	{
		const TranslationOption &origTransOpt = **iter;
		TranslationOption *newTransOpt = new TranslationOption(origTransOpt);
		Add(newTransOpt);
	}
}

TranslationOptionList::~TranslationOptionList()
{
	RemoveAllInColl(m_coll);
}

}

