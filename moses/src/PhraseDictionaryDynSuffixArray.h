#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

#include <map>

#include "PhraseDictionary.h"
#include "BilingualDynSuffixArray.h"

namespace Moses {

class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
	PhraseDictionaryDynSuffixArray(size_t m_numScoreComponent, PhraseDictionaryFeature* feature);
	~PhraseDictionaryDynSuffixArray();
	bool Load( const std::vector<FactorType>& m_input
						, const std::vector<FactorType>& m_output
						, std::string m_source
						, std::string m_target
						, std::string m_alignments
						, const std::vector<float> &m_weight
						, size_t m_tableLimit
						, const LMList &languageModels
						, float weightWP);
	// functions below required by base class
	void SetWeightTransModel(const std::vector<float, std::allocator<float> >&);
	const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
	void InitializeForInput(const InputType& i);
	void AddEquivPhrase(const Phrase &, const TargetPhrase &){}
	void CleanUp();
  void insertSnt(string&, string&, string&);
  void deleteSnt(unsigned, unsigned);
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
