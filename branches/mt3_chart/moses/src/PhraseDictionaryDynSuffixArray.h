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
	int m_sntIndex, m_startTarget, m_endTarget, m_startSource, m_endSource;
	PhrasePair(int sntIndex, int startTarget, int endTarget, int startSource, int endSource)
	: m_startTarget(startTarget)
	, m_endTarget(endTarget)
	, m_startSource(startSource)
	, m_endSource(endSource)
  , m_sntIndex(sntIndex)
	{}
};
	
class SentenceAlignment 
{
public:
  SentenceAlignment(int sntIndex, int sourceSize, int targetSize);
	int m_sntIndex;
	vector<wordID_t>* trgSnt;
  vector<wordID_t>* srcSnt;
  vector<int> alignedCountSrc;
  vector< vector<int> > alignedTrg;
	bool Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const;
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
  int loadCorpus(InputFileStream& corpus, vector<wordID_t>&, vector<wordID_t>&);
  int loadAlignments(InputFileStream& aligs);
  const int* getSntIndexes(vector<unsigned>&) const; 	
  TargetPhrase* getMosesFactorIDs(const PhrasePair&) const;
  bool getLocalVocabIDs(const Phrase&, vector<wordID_t>&) const;
	std::vector<float> m_weight;
	size_t m_tableLimit;
	const LMList *m_languageModels;
	float m_weightWP;
	
	std::map<const Factor *, wordID_t> vocabLookup_;
	std::map<wordID_t, const Factor *> vocabLookupRev_;	
	
	int GetSourceSentenceSize(int sentenceId) const
	{ return (sentenceId==srcSntBreaks_.size()-1) ? srcCrp_->size() - srcSntBreaks_[sentenceId] : srcSntBreaks_[sentenceId+1] - srcSntBreaks_[sentenceId]; }
	int GetTargetSentenceSize(int sentenceId) const
	{ return (sentenceId==trgSntBreaks_.size()-1) ? trgCrp_->size() - trgSntBreaks_[sentenceId] : trgSntBreaks_[sentenceId+1] - trgSntBreaks_[sentenceId]; }
	
};

} // end namespace
#endif
