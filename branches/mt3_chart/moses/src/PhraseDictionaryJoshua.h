
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

class PhraseDictionaryJoshua : public PhraseDictionary
{
	typedef PhraseDictionary MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryJoshua&);

protected:

public:
	PhraseDictionaryJoshua(size_t numScoreComponent)
	: MyBase(numScoreComponent)
	{}
	virtual ~PhraseDictionaryJoshua();

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
	virtual void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

	virtual const ChartRuleCollection *GetChartRuleCollection(
																					InputType const& src
																					,WordsRange const& range
																					,bool adhereTableLimit) const;

	void CleanUp();

};

} // namespace

