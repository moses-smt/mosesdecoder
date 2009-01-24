
#pragma once

#include <vector>
#include <map>
#include <iostream>
#include "TypeDef.h"
#include "../../moses/src/TypeDef.h"
#include "../../moses/src/Word.h"

class Moses::Factor;

namespace MosesOnDiskPt
{

class Word
{
	friend std::ostream& operator<<(std::ostream& out, const Word &word);

protected:
	std::vector<VocabId> m_factors;

public:
	Word()
	{}
	Word(size_t size)
		:m_factors(size)
	{}

	Word(size_t size, VocabId initVocab)
		:m_factors(size, initVocab)
	{}

	void resize(size_t newSize)
	{ m_factors.resize(newSize); }

	size_t GetSize() const
	{ return m_factors.size(); }
	size_t GetDiskSize() const
	{ return m_factors.size() * sizeof(VocabId);	}
	
	VocabId GetVocabId(size_t factorType) const
	{ return m_factors[factorType]; }
	
	void SetVocabId(size_t factorType, VocabId vocabId)
	{ m_factors[factorType] = vocabId; }
	void AddVocabId(VocabId vocabId)
	{ m_factors.push_back(vocabId); }

	bool operator< (const Word &compare) const
	{
		return m_factors < compare.m_factors;
	}
	bool operator> (const Word &compare) const
	{
		return m_factors > compare.m_factors;
	}
	bool operator== (const Word &compare) const
	{
		return m_factors == compare.m_factors;
	}

	void Save(std::ostream &outStream) const;
	void Load(std::istream &inStream, const std::vector<Moses::FactorType> &factorsVec);
	bool ConvertFromMoses(const std::vector<Moses::FactorType> &inputFactorsVec
											, const Moses::Word &origWord
											, const std::map<std::string, VocabId> &vocabLookup);
	Moses::Word ConvertToMoses(const std::vector<Moses::FactorType> &factors
														, const std::vector< std::vector<const Moses::Factor*> > &targetLookup) const;

};

}

