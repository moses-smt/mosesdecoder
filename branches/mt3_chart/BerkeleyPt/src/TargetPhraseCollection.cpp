
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

void TargetPhraseCollection::Save(Db &db, Moses::UINT32 sourceNodeId, int numScores, size_t sourceWordSize, size_t targetWordSize, int tableLimit) const
{
	size_t memUsed;
	char *mem = WriteToMemory(memUsed, numScores, sourceWordSize, targetWordSize, tableLimit);

	Dbt key(&sourceNodeId, sizeof(sourceNodeId));
	Dbt data(mem, memUsed);
	
	int ret = db.put(NULL, &key, &data, DB_NOOVERWRITE);
	assert(ret == 0);

	free(mem);
}

char *TargetPhraseCollection::WriteToMemory(size_t &memUsed, int numScores, size_t sourceWordSize, size_t targetWordSize, int tableLimit) const
{
	memUsed = sizeof(int);
	char *mem = (char*) malloc(memUsed);
	
	// size
	int sizeColl = (tableLimit == 0) ? m_coll.size() : min( (int) m_coll.size(), tableLimit);
	memcpy(mem, &sizeColl, memUsed);

	for (size_t ind = 0; ind < sizeColl; ++ind)
	{
		const TargetPhrase &phrase = *m_coll[ind];
				
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

