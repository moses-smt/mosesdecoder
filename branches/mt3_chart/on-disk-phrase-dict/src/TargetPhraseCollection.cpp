
#include <fstream>
#include "TargetPhraseCollection.h"
#include "../../moses/src/Util.h"

using namespace std;

namespace MosesOnDiskPt
{

TargetPhraseCollection::~TargetPhraseCollection()
{
	Moses::RemoveAllInColl(m_phraseColl);
}

const Phrase &TargetPhraseCollection::Add(const Phrase &phrase)
{
	pair<CollType::iterator, bool> ret;
	ret = m_phraseColl.insert(new Phrase(phrase));

	return **ret.first;
}

void TargetPhraseCollection::Save(const std::string &filePath)
{
	ofstream fileStream(filePath.c_str(), ios::out | ios::binary);

	// write num factors
	fileStream.write((char*) &m_numFactors, sizeof(m_numFactors));

	CollType::iterator iter;
	for (iter = m_phraseColl.begin(); iter != m_phraseColl.end(); ++iter)
	{
		Phrase &phrase = **iter;

		// write size of phrase
		long currPos = fileStream.tellp();

		phrase.Save(fileStream);
		phrase.SetBinOffset(currPos);
	}

	fileStream.close();

}

}

