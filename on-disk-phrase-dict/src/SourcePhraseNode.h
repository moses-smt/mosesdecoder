
#pragma once

#include <map>
#include <vector>
#include <iostream>
#include "TypeDef.h"
#include "Word.h"
#include "../../moses/src/Util.h"

namespace MosesOnDiskPt
{

class Phrase;
class TargetPhrase;
class TargetPhraseList;

class SourcePhraseNode
{
protected:
	typedef std::map<Word, SourcePhraseNode> NodeMap;
	NodeMap m_map;

	std::vector<TargetPhrase*> m_targetPhrases;

	long m_binOffset;

	const SourcePhraseNode *GetChild(const Word &searchWord
																									,std::ifstream &sourceFile
																									,const std::vector<Moses::FactorType> &inputFactorsVec
																									,size_t mapSize
																									,long startOffset) const;

public:
	SourcePhraseNode()
	{}

	SourcePhraseNode(long binOffset)
		:m_binOffset(binOffset)
	{}

	// only used when getting from node
	SourcePhraseNode(const SourcePhraseNode &copy)
		:m_binOffset(copy.m_binOffset)
	{}

	// only used when getting from node
	SourcePhraseNode& operator=(const SourcePhraseNode &copy)
	{
		if(this != &copy)
		{
			m_binOffset			= copy.m_binOffset;
		}
		return *this;
	}

	SourcePhraseNode &AddWord(const Word &word);
	~SourcePhraseNode()
	{
		Moses::RemoveAllInColl(m_targetPhrases);
	}

	void AddTarget(const Phrase &targetPhrase
								,const std::vector<float> &scores
								,const std::vector< std::pair<size_t, size_t> > &align);
	long GetBinOffset() const
	{ return m_binOffset; }

	void Save(std::ostream &outStream);

	const SourcePhraseNode *GetChild(const Word &searchWord
																	,std::ifstream &sourceFile
																	,const std::vector<Moses::FactorType> &inputFactorsVec) const;

	const TargetPhraseList *GetTargetPhraseCollection(
																				const std::vector<Moses::FactorType> &inputFactorsVec
																				,const std::vector<Moses::FactorType> &outputFactorsVec
																				,std::ifstream &sourceFile
																				,std::ifstream &targetFile
																				,size_t numScores) const;

};

} // namespace


