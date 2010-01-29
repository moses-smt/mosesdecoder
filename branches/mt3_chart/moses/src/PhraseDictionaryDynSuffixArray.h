#ifndef MOSES_DICTSFXARY_H
#define MOSES_DICTSFXARY_H

#include "PhraseDictionary.h"
#include "DynSuffixArray.h" 
#include "DynSAInclude/vocab.h"
#include "DynSAInclude/types.h"
#include "DynSAInclude/utils.h"
#include "InputFileStream.h"
namespace Moses {
	
class PhrasePair
{
public:
	int m_startE, m_endE, m_startF, m_endF;
	PhrasePair(int startE, int endE, int startF, int endF)
	: m_startE(startE)
	, m_endE(endE)
	, m_startF(startF)
	, m_endF(endF)
	{}
};
	
class SentenceAlignment 
{
public:
  SentenceAlignment(int size) {
    for(int i=0; i < size; ++i) {
      alignedCountSrc.push_back(0);
      vector<int> trgWrd;
      alignedTrg.push_back(trgWrd);
    }
  }
  vector<wordID_t>* trgSnt;
  vector<wordID_t>* srcSnt;
  vector<int> alignedCountSrc;
  vector< vector<int> > alignedTrg;
	bool Extract(int maxPhraseLength, vector<PhrasePair>&) const;
};
	
class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
  PhraseDictionaryDynSuffixArray(size_t numScoreComponent);
  ~PhraseDictionaryDynSuffixArray();
  
	bool Load(string source, string target, string alignments
						, const std::vector<float> &weight
						, size_t tableLimit
						, const LMList &languageModels
						, float weightWP);
	void LoadVocabLookup();
  void save(string);
  void load(string);
  // functions below required by base class
  PhraseTableImplementation GetPhraseTableImplementation() const { return dynSuffixArray; }
  void SetWeightTransModel(const std::vector<float, std::allocator<float> >&);
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  void InitializeForInput(const InputType& i);
  const ChartRuleCollection* GetChartRuleCollection(InputType const& src, WordsRange const& range,
    bool adhereTableLimit,const CellCollection &cellColl) const {}
  void CleanUp();
private:
  DynSuffixArray* srcSA_;
  DynSuffixArray* trgSA_;
  vector<wordID_t>* srcCrp_;
  vector<wordID_t>* trgCrp_;
  vector<unsigned> srcSntBreaks_, trgSntBreaks_;
  Vocab* vocab_;
  vector<SentenceAlignment> alignments_;
  int loadCorpus(InputFileStream* corpus, vector<wordID_t>&, vector<wordID_t>&);
  int loadAlignments(InputFileStream* aligs);
  const int* getSntIndexes(vector<unsigned>&) const; 	
	std::vector<float> m_weight;
	size_t m_tableLimit;
	const LMList *m_languageModels;
	float m_weightWP;
	
	std::map<const Factor *, wordID_t> vocabLookup_;
	std::map<wordID_t, const Factor *> vocabLookupRev_;	
};

} // end namespace
#endif
