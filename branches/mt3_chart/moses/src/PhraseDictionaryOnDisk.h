
#pragma once

#include <map>
#include <vector>
#include <string>
#include "PhraseDictionary.h"
#include "ChartRuleCollection.h"
#include "../../on-disk-phrase-dict/src/Word.h"

namespace MosesOnDiskPt
{
	class SourcePhraseNode;
};

namespace Moses
{

class ChartRuleCollection;

class PhraseDictionaryOnDisk : public PhraseDictionary
{
	typedef PhraseDictionary MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryOnDisk&);

protected:
	std::map<std::string, MosesOnDiskPt::VocabId> m_vocabLookup;
	std::vector< std::vector<const Factor*> > m_targetLookup;
		// ind = vocabId. inner ind = index on disk
	std::vector<float> m_weight;

	std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
	mutable std::ifstream m_sourceFile, m_targetFile;
	MosesOnDiskPt::SourcePhraseNode *m_initNode;

	mutable std::vector<TargetPhraseCollection*> m_cache;
	mutable std::vector<ChartRuleCollection*> m_chartTargetPhraseColl;
	mutable std::list<Phrase*> m_sourcePhrase;

	long SearchNode(long offset, const MosesOnDiskPt::Word &searchWord) const;
	void LoadTargetLookup();

public:
	PhraseDictionaryOnDisk(size_t numScoreComponent)
	: MyBase(numScoreComponent)
	{}
	virtual ~PhraseDictionaryOnDisk();

	PhraseTableImplementation GetPhraseTableImplementation() const
	{ return OnDisk; }

	bool Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, const std::string &filePath
								, const std::vector<float> &weight
								, size_t tableLimit);

	// PhraseDictionary impl
	// for mert
	void SetWeightTransModel(const std::vector<float> &weightT);
	//! find list of translations that can translates src. Only for phrase input
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const;
	//! Create entry for translation of source to targetPhrase
	virtual void AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase);

	virtual const ChartRuleCollection *GetChartRuleCollection(
																					InputType const& src
																					,WordsRange const& range
																					,bool adhereTableLimit
																					,const CellCollection &cellColl) const;

	void CleanUp();

};

} // namespace

