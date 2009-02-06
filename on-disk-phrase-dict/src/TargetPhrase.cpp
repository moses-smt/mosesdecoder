
#include "TargetPhrase.h"
#include "Phrase.h"
#include "../../moses/src/TargetPhrase.h"
#include "../../moses/src/TypeDef.h"

using namespace std;

namespace MosesOnDiskPt
{

void TargetPhrase::Save(std::ostream &outStream)
{
	// position of phrase in target.db
	long phraseOffset = m_targetPhrase.GetBinOffset();
	outStream.write((char*) &phraseOffset, sizeof(phraseOffset));
	
	// scores
	for (size_t ind = 0; ind < m_scores.size(); ++ind)
	{
		float score = m_scores[ind];
		outStream.write((char*) &score, sizeof(score));
	}

	// num of alignment info
	size_t numAlign = m_align.size();
	outStream.write((char*) &numAlign, sizeof(numAlign));

	for (size_t ind = 0; ind < m_align.size(); ++ind)
	{
		pair<size_t, size_t> &pair = m_align[ind];
		outStream.write((char*) &pair.first, sizeof(pair.first));
		outStream.write((char*) &pair.second, sizeof(pair.second));		
	}
}

void TargetPhrase::Load(std::istream &inStream, size_t numScores)
{
	// phrase offset already loaded

	// scores
	m_scores.resize(numScores);
	for (size_t ind = 0; ind < numScores; ++ind)
	{
		float score;
		inStream.read((char*) &score, sizeof(score));
		m_scores[ind] = score;
	}

	// num of alignment info
	size_t numAlign;
	inStream.read((char*) &numAlign, sizeof(numAlign));
	m_align.resize(numAlign);

	for (size_t ind = 0; ind < numAlign; ++ind)
	{
		size_t sourceAlign, targetAlign;
		inStream.read((char*) &sourceAlign, sizeof(sourceAlign));
		inStream.read((char*) &targetAlign, sizeof(targetAlign));		
		pair<size_t, size_t> pairAlign(sourceAlign, targetAlign);
		m_align[ind] = pairAlign;
	}

}

Moses::TargetPhrase *TargetPhrase::ConvertToMoses(const std::vector<Moses::FactorType> &factors
																									, const std::vector< std::vector<const Moses::Factor*> > &targetLookup
																									, const Moses::ScoreProducer &phraseDict
																									, const std::vector<float> &weightT
																									, float weightWP
																									, const Moses::LMList &lmList
																									, const Moses::Phrase &sourcePhrase
																									, size_t numSourceWordsPt) const
{
	Moses::TargetPhrase *ret = new Moses::TargetPhrase(Moses::Output);

	// source phrase
	ret->SetSourcePhrase(&sourcePhrase);

	// words
	for (size_t pos = 0; pos < m_targetPhrase.GetSize(); ++pos)
	{
		Moses::Word mosesWord = m_targetPhrase.GetWord(pos).ConvertToMoses(factors, targetLookup);
		ret->AddWord(mosesWord);
	}

	// scores
	ret->SetScore(&phraseDict, m_scores, weightT, weightWP, lmList);

	// alignments
	for (size_t ind = 0; ind < m_align.size(); ++ind)
	{
		const std::pair<size_t, size_t> &entry = m_align[ind];
		ret->AddAlignment(entry);
	}

	return ret;
}

} // namespace

