#ifndef MOSES_DICTSFXARY_H
#define MOSES_DICTSFXARY_H

#include "PhraseDictionary.h"
#include "DynSuffixArray.h" 
#include "DynSAInclude/vocab.h"
#include "DynSAInclude/types.h"
#include "DynSAInclude/utils.h"
#include "InputFileStream.h"
namespace Moses {

class SAPhrase
{
public:
	vector<wordID_t> words;		
	
	SAPhrase(size_t phraseSize)
	:words(phraseSize)
	{}
	
	void SetId(size_t pos, wordID_t id)
	{
    assert(pos < words.size());
		words[pos] = id;
	}
	bool operator<(const SAPhrase& phr2) const
  { return words < phr2.words; }
};

class PhrasePair
{
public:
	int m_startTarget, m_endTarget, m_startSource, m_endSource, m_sntIndex;
	PhrasePair(int startTarget, int endTarget, int startSource, int endSource, int sntIndex)
	: m_startTarget(startTarget)
	, m_endTarget(endTarget)
	, m_startSource(startSource)
	, m_endSource(endSource)
  , m_sntIndex(sntIndex)
	{}

	size_t GetTargetSize() const
	{ return m_endTarget - m_startTarget + 1; }
};
	
class SentenceAlignment 
{
public:
  SentenceAlignment(int sntIndex, int sourceSize, int targetSize);
	int m_sntIndex;
	vector<wordID_t>* trgSnt;
  vector<wordID_t>* srcSnt;
  vector<int> alignedCountTrg; 
  vector< vector<int> > alignedSrc; 
	bool Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const;
};

struct ScoresComp {
  bool operator()(const Scores& s1, const Scores& s2) const { 
    float score1(1), score2(1);
    iterate(s1, itr) score1 *= *itr;
    iterate(s2, itr) score2 *= *itr;
    return score1 < score2;
  }
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
    bool adhereTableLimit,const CellCollection &cellColl) const {return 0;}
  void CleanUp();
private:
  DynSuffixArray* srcSA_;
  DynSuffixArray* trgSA_;
  vector<wordID_t>* srcCrp_;
  vector<wordID_t>* trgCrp_;
  vector<unsigned> srcSntBreaks_, trgSntBreaks_;
  Vocab* vocab_;
  vector<SentenceAlignment> alignments_;
	std::vector<float> m_weight;
	size_t m_tableLimit;
	const LMList *m_languageModels;
	float m_weightWP;
	std::map<const Factor *, wordID_t> vocabLookup_;
	std::map<wordID_t, const Factor *> vocabLookupRev_;	
  mutable std::map<pair<wordID_t, wordID_t>, float> wordPairCache_; 
  const int maxSampleSize_;
  int loadCorpus(InputFileStream& corpus, vector<wordID_t>&, vector<wordID_t>&);
  int loadAlignments(InputFileStream& aligs);
  vector<unsigned> sampleSelection(vector<unsigned>) const;
  vector<int> getSntIndexes(vector<unsigned>&, const int) const; 	
  TargetPhrase* getMosesFactorIDs(const SAPhrase&) const;
  SAPhrase phraseFromSntIdx(const PhrasePair&) const;
  bool getLocalVocabIDs(const Phrase&, SAPhrase &output) const;
  void cacheWordProbs(wordID_t) const;
  float getLexicalWeight(const PhrasePair&) const;
	int GetSourceSentenceSize(size_t sentenceId) const
	{ return (sentenceId==srcSntBreaks_.size()-1) ? srcCrp_->size() - srcSntBreaks_.at(sentenceId) : srcSntBreaks_.at(sentenceId+1) - srcSntBreaks_.at(sentenceId); }
	int GetTargetSentenceSize(size_t sentenceId) const
	{ return (sentenceId==trgSntBreaks_.size()-1) ? trgCrp_->size() - trgSntBreaks_.at(sentenceId) : trgSntBreaks_.at(sentenceId+1) - trgSntBreaks_.at(sentenceId); }
};

} // end namespace
#endif
