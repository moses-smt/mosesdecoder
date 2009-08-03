
#include "TargetPhraseCollection.h"
#include "../../moses/src/Util.h"

using namespace std;

namespace MosesBerkeleyPt
{
TargetPhraseCollection::~TargetPhraseCollection()
{
	Moses::RemoveAllInColl(m_coll);
}

void TargetPhraseCollection::Save(Db &db, long sourceNodeId, int numScores, size_t sourceWordSize, size_t targetWordSize) const
{
	size_t totalMemUsed = 0;
	char *totalMem = NULL;

	vector<const TargetPhrase*>::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const TargetPhrase &phrase = **iter;
		
		size_t memUsed;
		char *mem = phrase.WriteOtherInfoToMemory(memUsed, numScores, sourceWordSize, targetWordSize);
		
		totalMem = (char*) realloc(totalMem, totalMemUsed + memUsed);
		memcpy(totalMem + totalMemUsed, mem, memUsed);

		totalMemUsed += memUsed;
	}
}


};

