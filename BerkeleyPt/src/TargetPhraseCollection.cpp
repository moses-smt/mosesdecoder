
#include <db_cxx.h>
#include "TargetPhraseCollection.h"
#include "../../moses/src/Util.h"

using namespace std;

namespace MosesBerkeleyPt
{

TargetPhraseCollection::~TargetPhraseCollection()
{
	Moses::RemoveAllInColl(m_coll);
}

void TargetPhraseCollection::Save(Db &db, Moses::UINT32 sourceNodeId, int numScores, size_t sourceWordSize, size_t targetWordSize) const
{
	size_t memUsed;
	char *mem = WriteToMemory(memUsed, numScores, sourceWordSize, targetWordSize);

	Dbt key(&sourceNodeId, sizeof(sourceNodeId));
	Dbt data(mem, memUsed);
	
	int ret = db.put(NULL, &key, &data, DB_NOOVERWRITE);
	assert(ret == 0);

	free(mem);
}

char *TargetPhraseCollection::WriteToMemory(size_t &memUsed, int numScores, size_t sourceWordSize, size_t targetWordSize) const
{
	memUsed = sizeof(int);
	char *mem = (char*) malloc(memUsed);
	
	// size
	int sizeColl = m_coll.size();
	memcpy(mem, &sizeColl, memUsed);

	vector<const TargetPhrase*>::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const TargetPhrase &phrase = **iter;
		
		size_t memUsed1;
		char *mem1 = phrase.WriteOtherInfoToMemory(memUsed1, numScores, sourceWordSize, targetWordSize);
		
		mem = (char*) realloc(mem, memUsed + memUsed1);
		memcpy(mem + memUsed, mem1, memUsed1);
		free(mem1);

		memUsed += memUsed1;
	}
	
	return mem;
}

class TargetPhraseOrderBy1stScore
	{
	public:	
		bool operator()(const TargetPhrase* a, const TargetPhrase *b) const
		{
			return a->GetScores()[0] > b->GetScores()[0];
		}
	};

void TargetPhraseCollection::Sort()
{
	std::sort(m_coll.begin(), m_coll.end(), TargetPhraseOrderBy1stScore());
}
	
};

