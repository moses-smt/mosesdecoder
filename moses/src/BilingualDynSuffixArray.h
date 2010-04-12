#ifndef moses_BilingualDynSuffixArray_h
#define moses_BilingualDynSuffixArray_h

#include "TargetPhrase.h"
#include "DynSuffixArray.h" 
#include "DynSAInclude/vocab.h"
#include "DynSAInclude/types.h"
#include "DynSAInclude/utils.h"
#include "InputFileStream.h"

namespace Moses {

class SAPhrase
{
public:
	std::vector<wordID_t> words;		
	
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
	std::vector<wordID_t>* trgSnt;
	std::vector<wordID_t>* srcSnt;
	std::vector<int> numberAligned; 
	std::vector< std::vector<int> > alignedList; 
	bool Extract(int maxPhraseLength, std::vector<PhrasePair*> &ret, int startSource, int endSource) const;
};
class ScoresComp {
public: 
	ScoresComp(const std::vector<float>& weights): m_weights(weights) {}
	bool operator()(const Scores& s1, const Scores& s2) const { 
		float score1(1), score2(1);
		int idx1(0), idx2(0);
		iterate(s1, itr) score1 += (*itr * m_weights.at(idx1++)); 
		iterate(s2, itr) score2 += (*itr * m_weights.at(idx2++));
		return score1 < score2;
	}
private: 
	const std::vector<float>& m_weights;
};
	
class BilingualDynSuffixArray {
public: 
	BilingualDynSuffixArray();
	~BilingualDynSuffixArray();
	bool Load(string source, string target, string alignments, 
						const std::vector<float> &weight);
	void LoadVocabLookup();
	void save(string);
	void load(string);
	void GetTargetPhrasesByLexicalWeight(const Phrase& src, std::vector< std::pair<Scores, TargetPhrase*> >& target) const;
	void CleanUp();
private:
	DynSuffixArray* m_srcSA;
	DynSuffixArray* m_trgSA;
	std::vector<wordID_t>* m_srcCorpus;
	std::vector<wordID_t>* m_trgCorpus;

	std::vector<unsigned> m_srcSntBreaks, m_trgSntBreaks;

	Vocab* m_vocab;
	ScoresComp* m_scoreCmp;

	std::vector<SentenceAlignment> m_alignments;
	std::vector<std::vector<short> > m_rawAlignments;

	std::map<const Factor *, wordID_t> m_vocabLookup;
	std::map<wordID_t, const Factor *> m_vocabLookupRev;	

	mutable std::map<std::pair<wordID_t, wordID_t>, std::pair<float, float> > m_wordPairCache; 
	const size_t m_maxPhraseLength, m_maxSampleSize;

	int loadCorpus(InputFileStream&, std::vector<wordID_t>&, std::vector<wordID_t>&);
	int loadAlignments(InputFileStream& aligs);
	int loadRawAlignments(InputFileStream& aligs);

	bool extractPhrases(const int&, const int&, const int&, std::vector<PhrasePair*>&, bool=false) const;
	SentenceAlignment getSentenceAlignment(const int, bool=false) const; 
	std::vector<unsigned> sampleSelection(std::vector<unsigned>) const;

	std::vector<int> getSntIndexes(std::vector<unsigned>&, const int) const;	
	TargetPhrase* getMosesFactorIDs(const SAPhrase&) const;
	SAPhrase trgPhraseFromSntIdx(const PhrasePair&) const;
	bool getLocalVocabIDs(const Phrase&, SAPhrase &) const;
	void cacheWordProbs(wordID_t) const;
	std::pair<float, float> getLexicalWeight(const PhrasePair&) const;

	int GetSourceSentenceSize(size_t sentenceId) const
	{ 
		return (sentenceId==m_srcSntBreaks.size()-1) ? 
			m_srcCorpus->size() - m_srcSntBreaks.at(sentenceId) : 
			m_srcSntBreaks.at(sentenceId+1) - m_srcSntBreaks.at(sentenceId); 
	}
	int GetTargetSentenceSize(size_t sentenceId) const
	{ 
		return (sentenceId==m_trgSntBreaks.size()-1) ?
			m_trgCorpus->size() - m_trgSntBreaks.at(sentenceId) : 
			m_trgSntBreaks.at(sentenceId+1) - m_trgSntBreaks.at(sentenceId); 
	}
};
} // end namespace
#endif
