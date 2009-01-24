
#pragma once

#include <iostream>
#include <vector>
#include "TypeDef.h"
#include "../../moses/src/TypeDef.h"
#include "../../moses/src/TargetPhrase.h"

class Moses::Factor;

namespace MosesOnDiskPt
{

class Phrase;

class TargetPhrase
{
protected:
	const Phrase &m_targetPhrase;
	std::vector<float> m_scores;
	std::vector< std::pair<size_t, size_t> > m_align;
public:
	TargetPhrase(const Phrase &targetPhrase)
		:m_targetPhrase(targetPhrase)
	{}

	TargetPhrase(const Phrase &targetPhrase
								,const std::vector<float> &scores
								,const std::vector< std::pair<size_t, size_t> > &align)
	:m_targetPhrase(targetPhrase)
	,m_scores(scores)
	,m_align(align)
	{}

	void Save(std::ostream &outStream);
	void Load(std::istream &inStream, size_t numScores);

	Moses::TargetPhrase *ConvertToMoses(const std::vector<Moses::FactorType> &factors
																		, const std::vector< std::vector<const Moses::Factor*> > &targetLookup
																		, const Moses::ScoreProducer &phraseDict
																		, const std::vector<float> &weightT
																		, float weightWP
																		, const Moses::LMList &lmList
																		, const Moses::Phrase &sourcePhrase
																		, size_t numSourceWordsPt) const;
};

}

