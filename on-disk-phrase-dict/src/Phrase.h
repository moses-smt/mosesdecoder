
#pragma once

#include <string>
#include <vector>
#include <map>
#include "TypeDef.h"
#include "Word.h"

namespace MosesOnDiskPt
{
class Vocab;

class Phrase
{
protected:
	std::vector<Word> m_vecPhrase;
	long m_binOffset;

public:
	Phrase()
	{}

	// for Moses format
	Phrase(const std::string &str
				, size_t numFactors
				, Vocab &vocab);

	// for Hiero format
	Phrase(const std::string &str
				, size_t numFactors
				, Vocab &vocab
				, std::map<size_t, size_t> &align);

	size_t GetSize() const
	{ return m_vecPhrase.size(); }
	const Word &GetWord(size_t pos) const
	{ return m_vecPhrase[pos]; }

	/** transitive comparison between 2 phrases
	*		used to insert & find phrase in dictionary
	*/
	bool operator< (const Phrase &compare) const
	{
		return m_vecPhrase < compare.m_vecPhrase;
	}

	long GetBinOffset() const
	{ return m_binOffset; }
	void SetBinOffset(long binOffset)
	{ m_binOffset = binOffset; }

	void Save(std::ostream &outStream) const;
	void Load(std::istream &inStream, const std::vector<Moses::FactorType> &factorsVec);

};

}

