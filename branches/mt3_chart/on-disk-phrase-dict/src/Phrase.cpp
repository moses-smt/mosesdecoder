#include <vector>
#include <string>
#include "Phrase.h"
#include "Vocab.h"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/Util.h"

using namespace std;

namespace MosesOnDiskPt
{

Phrase::Phrase(const std::string &str
			, size_t numFactors
			, Vocab &vocab)
{
	std::vector< std::vector<std::string> > vecStr = Moses::Phrase::Parse(str, "|");
	m_vecPhrase.resize(vecStr.size());

	for (size_t pos = 0; pos < vecStr.size(); ++pos)
	{
		const std::vector<std::string> &wordStr = vecStr[pos];
		assert(wordStr.size() == numFactors);

		Word &word = m_vecPhrase[pos];
		word.resize(numFactors);

		for (size_t factorType = 0; factorType < numFactors; ++factorType)
		{
			VocabId vocabId;
			const string &factorStr = wordStr[factorType];
			vocabId = vocab.AddFactor(factorStr);
			word.SetVocabId(factorType, vocabId);
		}
	}
}

Phrase::Phrase(const std::string &str
				, size_t numFactors
				, Vocab &vocab
				, std::map<size_t, size_t> &align)
{
	std::vector< std::vector<std::string> > vecStr = Moses::Phrase::Parse(str, "|");
	m_vecPhrase.resize(vecStr.size());

	for (size_t pos = 0; pos < vecStr.size(); ++pos)
	{
		const std::vector<std::string> &wordStr = vecStr[pos];
		assert(wordStr.size() == numFactors);

		Word &word = m_vecPhrase[pos];
		word.resize(numFactors);

		for (size_t factorType = 0; factorType < numFactors; ++factorType)
		{
			VocabId vocabId;
			const string &factorStr = wordStr[factorType];
			if (factorStr.substr(0, 3) == "[X,")
			{
				vocabId = vocab.AddFactor("[X]");

				// flag alignment
				string indAlignStr = factorStr.substr(3, factorStr.size() - 4);
				size_t indAlign = Moses::Scan<size_t>(indAlignStr);
				assert(align.find(indAlign) == align.end());

				align[indAlign] = pos;
			}
			else
			{
				vocabId = vocab.AddFactor(factorStr);
			}
			word.SetVocabId(factorType, vocabId);
		}
	}
}

void Phrase::Save(std::ostream &outStream) const
{
	size_t phraseSize = GetSize();
	outStream.write((char*) &phraseSize, sizeof(phraseSize));

	for (size_t pos = 0; pos < phraseSize; ++pos)
	{
		const Word &word = GetWord(pos);
		word.Save(outStream);
	}

}

void Phrase::Load(std::istream &inStream, const std::vector<Moses::FactorType> &factorsVec)
{
	size_t phraseSize;
	inStream.read((char*) &phraseSize, sizeof(phraseSize));
	m_vecPhrase.resize(phraseSize);

	for (size_t pos = 0; pos < phraseSize; ++pos)
	{
		Word &word = m_vecPhrase[pos];
		word.resize(factorsVec.size());
		word.Load(inStream, factorsVec);
	}
}

}


