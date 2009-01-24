
#include "TargetPhraseList.h"
#include "TargetPhrase.h"
#include "Phrase.h"
#include "../../moses/src/Util.h"

namespace MosesOnDiskPt
{

TargetPhraseList::~TargetPhraseList()
{
	Moses::RemoveAllInColl(m_phraseCache);
	Moses::RemoveAllInColl(m_coll);
}

};



