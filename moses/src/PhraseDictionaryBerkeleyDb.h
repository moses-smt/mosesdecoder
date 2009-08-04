
#pragma once

#include <map>
#include <vector>
#include <string>
#include "PhraseDictionary.h"
#include "ChartRuleCollection.h"
#include "../../BerkeleyPt/src/DbWrapper.h"
#include "../../BerkeleyPt/src/Word.h"
#include "../../BerkeleyPt/src/SourcePhraseNode.h"

namespace Moses
{
class TargetPhraseCollection;
class ProcessedRuleStackBerkeleyDb;

class PhraseDictionaryBerkeleyDb : public PhraseDictionary
{
	typedef PhraseDictionary MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryBerkeleyDb&);
	
protected:
	std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
	std::vector<float> m_weight;

	MosesBerkeleyPt::DbWrapper m_dbWrapper;
	MosesBerkeleyPt::SourcePhraseNode *m_initNode;

	mutable std::vector<TargetPhraseCollection*> m_cache;
	mutable std::vector<ChartRuleCollection*> m_chartTargetPhraseColl;
	mutable std::vector<ProcessedRuleStackBerkeleyDb*>	m_runningNodesVec;

	void LoadTargetLookup();
	
public:
	PhraseDictionaryBerkeleyDb(size_t numScoreComponent)
	: MyBase(numScoreComponent)
	{}
	virtual ~PhraseDictionaryBerkeleyDb();

	PhraseTableImplementation GetPhraseTableImplementation() const
	{ return BerkeleyDb; }

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
	
	void InitializeForInput(const InputType& input);
	void CleanUp();
	
};


};


