#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

#include <map>

#include "PhraseDictionary.h"
#include "BilingualDynSuffixArray.h"

namespace Moses {

class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
	PhraseDictionaryDynSuffixArray(size_t numScoreComponent, PhraseDictionaryFeature* feature);
	~PhraseDictionaryDynSuffixArray();
	bool Load(std::string source, std::string target, std::string alignments
						, const std::vector<float> &weight
						, size_t tableLimit
						, const LMList &languageModels
						, float weightWP);
	// functions below required by base class
	void SetWeightTransModel(const std::vector<float, std::allocator<float> >&);
	const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
	void InitializeForInput(const InputType& i);
	void AddEquivPhrase(const Phrase &, const TargetPhrase &){}
	void CleanUp();
private:
	BilingualDynSuffixArray *m_biSA;
	std::vector<float> m_weight;
	size_t m_tableLimit;
	const LMList *m_languageModels;
	float m_weightWP;
	
	virtual const ChartRuleCollection *GetChartRuleCollection(InputType const& src, WordsRange const& range,
																														bool adhereTableLimit,const CellCollection &cellColl) const;
	

};

} // end namespace
#endif
