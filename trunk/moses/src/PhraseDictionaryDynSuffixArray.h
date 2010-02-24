#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

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
  vector<int> numberAligned; 
  vector< vector<int> > alignedList; 
	bool Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const;
};
class ScoresComp {
public: 
  ScoresComp(const vector<float>& weights): m_weights(weights) {}
  bool operator()(const Scores& s1, const Scores& s2) const { 
    float score1(1), score2(1);
    int idx1(0), idx2(0);
    iterate(s1, itr) score1 += (*itr * m_weights.at(idx1++)); 
    iterate(s2, itr) score2 += (*itr * m_weights.at(idx2++));
    return score1 < score2;
  }
private: 
  const vector<float>& m_weights;
};
	
class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
  PhraseDictionaryDynSuffixArray(size_t numScoreComponent, PhraseDictionaryFeature* feature);
  ~PhraseDictionaryDynSuffixArray();
	bool Load(string source, string target, string alignments
						, const vector<float> &weight
						, size_t tableLimit
						, const LMList &languageModels
						, float weightWP);
	void LoadVocabLookup();
  void save(string);
  void load(string);
  // functions below required by base class
  void SetWeightTransModel(const vector<float, std::allocator<float> >&);
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  void InitializeForInput(const InputType& i);
  void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase){}
  void CleanUp();
private:
  DynSuffixArray* srcSA_;
  DynSuffixArray* trgSA_;
  vector<wordID_t>* srcCrp_;
  vector<wordID_t>* trgCrp_;
  vector<unsigned> srcSntBreaks_, trgSntBreaks_;
  Vocab* vocab_;
  ScoresComp* scoreCmp_;
  vector<SentenceAlignment> alignments_;
  vector<vector<short> > rawAlignments_;
	vector<float> m_weight;
	size_t m_tableLimit;
	const LMList *m_languageModels;
	float m_weightWP;
	std::map<const Factor *, wordID_t> vocabLookup_;
	std::map<wordID_t, const Factor *> vocabLookupRev_;	
  mutable std::map<pair<wordID_t, wordID_t>, pair<float, float> > wordPairCache_; 
  const int maxPhraseLength_, maxSampleSize_;
  int loadCorpus(InputFileStream&, vector<wordID_t>&, vector<wordID_t>&);
  int loadAlignments(InputFileStream& aligs);
  int loadRawAlignments(InputFileStream& aligs);
  bool extractPhrases(const int&, const int&, const int&, vector<PhrasePair*>&, bool=false) const;
  SentenceAlignment getSentenceAlignment(const int, bool=false) const; 
  vector<unsigned> sampleSelection(vector<unsigned>) const;
  vector<int> getSntIndexes(vector<unsigned>&, const int) const; 	
  TargetPhrase* getMosesFactorIDs(const SAPhrase&) const;
  SAPhrase trgPhraseFromSntIdx(const PhrasePair&) const;
  bool getLocalVocabIDs(const Phrase&, SAPhrase &) const;
  void cacheWordProbs(wordID_t) const;
  pair<float, float> getLexicalWeight(const PhrasePair&) const;
	int GetSourceSentenceSize(size_t sentenceId) const
	{ return (sentenceId==srcSntBreaks_.size()-1) ? srcCrp_->size() - srcSntBreaks_.at(sentenceId) : srcSntBreaks_.at(sentenceId+1) - srcSntBreaks_.at(sentenceId); }
	int GetTargetSentenceSize(size_t sentenceId) const
	{ return (sentenceId==trgSntBreaks_.size()-1) ? trgCrp_->size() - trgSntBreaks_.at(sentenceId) : trgSntBreaks_.at(sentenceId+1) - trgSntBreaks_.at(sentenceId); }
};
} // end namespace
#endif
