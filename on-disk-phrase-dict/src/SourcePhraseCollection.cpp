
#include <fstream>
#include <cassert>
#include "SourcePhraseCollection.h"

using namespace std;

namespace MosesOnDiskPt
{

SourcePhraseNode &SourcePhraseCollection::Add(const Phrase &phrase)
{
	SourcePhraseNode *node = &m_coll;
	for (size_t pos = 0; pos < phrase.GetSize(); ++pos)
	{
		const Word &word = phrase.GetWord(pos);
		node = &node->AddWord(word);
	}
	assert(node != NULL);
	return *node;
}

void SourcePhraseCollection::Save(const std::string &filePath, size_t numScores)
{
	ofstream fileStream(filePath.c_str(), ios::out | ios::binary);

	// space for init offset at beginning of file
	long initOffset = 666999;
	fileStream.write((char*) &initOffset, sizeof(initOffset));

	// write num factors
	fileStream.write((char*) &m_numFactors, sizeof(m_numFactors));

	// write num scores
	fileStream.write((char*) &numScores, sizeof(numScores));

	// recursively write tree
	m_coll.Save(fileStream);

	// initial offset
	initOffset = m_coll.GetBinOffset();
	fileStream.seekp(0, ios::beg);
	fileStream.write((char*) &initOffset, sizeof(initOffset));

	// must close or go to end of file again
	fileStream.close();

}

} // namespace

