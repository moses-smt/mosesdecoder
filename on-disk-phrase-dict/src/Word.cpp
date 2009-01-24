
#include <string>
#include "Word.h"
#include "../../moses/src/Factor.h"

using namespace std;

namespace MosesOnDiskPt
{

void Word::Save(std::ostream &outStream) const
{
	for (size_t factorType = 0; factorType < GetSize(); ++factorType)
	{
		VocabId vocabId = GetVocabId(factorType);
		outStream.write((char*) &vocabId, sizeof(vocabId));
	}
}

void Word::Load(std::istream &inStream, const std::vector<Moses::FactorType> &factorsVec)
{
	// create word found on disk
	for (size_t ind = 0 ; ind < factorsVec.size() ; ++ind)
	{
		size_t factorType = factorsVec[ind];
		VocabId vocabId;
		inStream.read((char*) &vocabId, sizeof(vocabId));
		SetVocabId(ind, vocabId);
	}

}

bool Word::ConvertFromMoses(const std::vector<Moses::FactorType> &inputFactorsVec
														, const Moses::Word &origWord
														, const std::map<std::string, VocabId> &vocabLookup)
{
	for (size_t ind = 0 ; ind < inputFactorsVec.size() ; ++ind)
	{
		size_t factorType = inputFactorsVec[ind];

		const Moses::Factor *factor = origWord.GetFactor(factorType);
		if (factor != NULL)
		{
			const string &str = factor->GetString();
			map<std::string, VocabId>::const_iterator iterVocabLookup = vocabLookup.find(str);
			if (iterVocabLookup == vocabLookup.end())
			{ // factor not in phrase table -> phrse definately not in. exit
				return false;
			}
			else
			{
				VocabId vocabId = iterVocabLookup->second;
				SetVocabId(ind, vocabId);
			}
		} // if (factor
	} // for (size_t factorType

	return true;
}

Moses::Word Word::ConvertToMoses(const std::vector<Moses::FactorType> &factors
																 , const std::vector< std::vector<const Moses::Factor*> > &targetLookup) const
{
	Moses::Word ret;

	vector<Moses::FactorType>::const_iterator iter;
	for (size_t ind = 0; ind < factors.size(); ++ind)
	{
		VocabId vocabId = m_factors[ind];
		Moses::FactorType factorType = factors[ind];
		const Moses::Factor *factor = targetLookup[vocabId][ind];
		assert(factor);

		ret.SetFactor(factorType, factor);
	}


	return ret;
}

std::ostream& operator<<(std::ostream& out, const Word &word)
{
	out << "(";
	for (size_t ind = 0; ind < word.m_factors.size(); ++ind)
		out << word.GetVocabId(ind) << " ";
	out << ")";
	return out;
}

} // namespace

