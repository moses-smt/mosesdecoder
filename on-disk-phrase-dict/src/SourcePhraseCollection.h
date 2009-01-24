
#include "SourcePhraseNode.h"
#include "Phrase.h"
#include "TypeDef.h"

namespace MosesOnDiskPt
{

class SourcePhraseCollection
{
protected:
	SourcePhraseNode m_coll;
	size_t m_numFactors;

public:
	SourcePhraseCollection(size_t numFactors)
		:m_numFactors(numFactors)
	{}

	SourcePhraseNode &Add(const Phrase &phrase);
	void Save(const std::string &filePath, size_t numScores);

};

}

