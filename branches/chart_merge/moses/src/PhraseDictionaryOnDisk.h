
#pragma once

#include <map>
#include <vector>
#include <string>
#include "PhraseDictionary.h"
#include "ChartRuleCollection.h"
#include "../../OnDiskPt/src/OnDiskWrapper.h"
#include "../../OnDiskPt/src/Word.h"
#include "../../OnDiskPt/src/PhraseNode.h"

namespace Moses
{
class TargetPhraseCollection;
class ProcessedRuleStackOnDisk;
class CellCollection;

class PhraseDictionaryOnDisk : public PhraseDictionary
{
	typedef PhraseDictionary MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryOnDisk&);
	
protected:
	std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
	std::vector<float> m_weight;
	std::string m_filePath;
	
	mutable OnDiskPt::OnDiskWrapper m_dbWrapper;

	mutable std::map<UINT64, const TargetPhraseCollection*> m_cache;
	mutable std::vector<ChartRuleCollection*> m_chartTargetPhraseColl;
	mutable std::list<Phrase*> m_sourcePhrase;
	mutable std::list<const OnDiskPt::PhraseNode*> m_sourcePhraseNode;

	mutable std::vector<ProcessedRuleStackOnDisk*>	m_runningNodesVec;
	
	void LoadTargetLookup();
	
public:
	PhraseDictionaryOnDisk(size_t numScoreComponent, PhraseDictionaryFeature* feature)
	: MyBase(numScoreComponent, feature)
	{}
	virtual ~PhraseDictionaryOnDisk();

	PhraseTableImplementation GetPhraseTableImplementation() const
	{ return BerkeleyDb; }

	bool Load(const std::vector<FactorType> &input
						, const std::vector<FactorType> &output
						, const std::string &filePath
						, const std::vector<float> &weight
						, size_t tableLimit);
	
	std::string GetScoreProducerDescription() const
	{ return "BerkeleyPt"; }

	// PhraseDictionary impl
	// for mert
	void SetWeightTransModel(const std::vector<float> &weightT);
	//! find list of translations that can translates src. Only for phrase input
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const;

	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

	//! Create entry for translation of source to targetPhrase
	virtual void AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase);
	
	virtual const ChartRuleCollection *GetChartRuleCollection(InputType const& src, WordsRange const& range,
																														bool adhereTableLimit,const CellCollection &cellColl) const;
	
	void InitializeForInput(const InputType& input);
	void CleanUp();
	
};


};


