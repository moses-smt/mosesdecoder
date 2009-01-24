
#pragma once

#include <set>
#include "Phrase.h"
#include "TypeDef.h"

namespace MosesOnDiskPt
{

class TargetPhraseCollection
{
protected:
	typedef std::set<Phrase*> CollType;
	CollType m_phraseColl;

	size_t m_numFactors;
public:
	TargetPhraseCollection(size_t numFactors)
		:m_numFactors(numFactors)
	{}
	virtual ~TargetPhraseCollection();

	const Phrase &Add(const Phrase &phrase);
	void Save(const std::string &filePath);
};

}

