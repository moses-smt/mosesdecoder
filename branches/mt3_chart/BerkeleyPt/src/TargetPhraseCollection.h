#pragma once

#include <vector>
#include "TargetPhrase.h"

namespace MosesBerkeleyPt
{

class TargetPhraseCollection
{
protected:
	std::vector<const TargetPhrase*> m_coll;

	char *WriteToMemory(size_t &totalMemUsed, int numScores, size_t sourceWordSize, size_t targetWordSize) const;
public:
	TargetPhraseCollection()
	{}
	~TargetPhraseCollection();

	void AddTargetPhrase(const TargetPhrase *phrase)
	{
		m_coll.push_back(phrase);
	}

	void Save(Db &db, long sourceNodeId, int numScores, size_t sourceWordSize, size_t targetWordSize) const;

};

};
