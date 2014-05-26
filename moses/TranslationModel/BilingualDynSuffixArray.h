#ifndef moses_BilingualDynSuffixArray_h
#define moses_BilingualDynSuffixArray_h

#include "DynSuffixArray.h"
#include "moses/TranslationModel/DynSAInclude/vocab.h"
#include "moses/TranslationModel/DynSAInclude/types.h"
#include "moses/TranslationModel/DynSAInclude/utils.h"
#include "moses/TranslationModel/WordCoocTable.h"
#include "moses/InputFileStream.h"
#include "moses/FactorTypeSet.h"
#include "moses/TargetPhrase.h"
#include <boost/dynamic_bitset.hpp>
#include "moses/TargetPhraseCollection.h"
#include <map>

using namespace std;
namespace Moses
{
class PhraseDictionaryDynSuffixArray;

/** @todo ask Abbey Levenberg
 */
class SAPhrase
{
public:
  vector<wordID_t> words;

  SAPhrase(size_t phraseSize)
    :words(phraseSize) {
  }

  void SetId(size_t pos, wordID_t id) {
    words.at(pos) = id;
  }
  bool operator<(const SAPhrase& phr2) const {
    return words < phr2.words;
  }
};

/** @todo ask Abbey Levenberg
 */
class PhrasePair
{
public:
  int m_startTarget, m_endTarget, m_startSource, m_endSource, m_sntIndex;
  PhrasePair(int startTarget, int endTarget, int startSource, int endSource, int sntIndex)
    : m_startTarget(startTarget)
    , m_endTarget(endTarget)
    , m_startSource(startSource)
    , m_endSource(endSource)
    , m_sntIndex(sntIndex) {
  }

  size_t GetTargetSize() const {
    return m_endTarget - m_startTarget + 1;
  }

  size_t GetSourceSize() const {
    return m_endSource - m_startSource + 1;
  }
};

/** @todo ask Abbey Levenberg
 */
class SentenceAlignment
{
public:
  SentenceAlignment(int sntIndex, int sourceSize, int targetSize);
  int m_sntIndex;
  vector<wordID_t>* trgSnt;
  vector<wordID_t>* srcSnt;
  vector<int> numberAligned;
  vector< vector<int> > alignedList;
  bool Extract(int maxPhraseLength, vector<PhrasePair*> &ret,
               int startSource, int endSource) const;
};

class ScoresComp
{
public:
  ScoresComp(const vector<float>& weights)
  {}
  bool operator()(const Scores& s1, const Scores& s2) const {
    return s1[0] < s2[0]; // just p(e|f) as approximation
    // float score1(0), score2(0);
    // int idx1(0), idx2(0);
    // for (Scores::const_iterator itr = s1.begin();
    // 	 itr != s1.end(); ++itr) {
    //   score1 += log(*itr * m_weights.at(idx1++));
    // }
    // for (Scores::const_iterator itr = s2.begin();
    // 	 itr != s2.end(); ++itr) {
    //   score2 += log(*itr * m_weights.at(idx2++));
    // }
    // return score1 < score2;
  }
};

struct BetterPhrase {
  ScoresComp const& cmp;
  BetterPhrase(ScoresComp const& sc);
  // bool operator()(pair<Scores, TargetPhrase const*> const& a,
  // pair<Scores, TargetPhrase const*> const& b) const;
  bool operator()(pair<Scores, SAPhrase const*> const& a,
                  pair<Scores, SAPhrase const*> const& b) const;
};

/** @todo ask Abbey Levenberg
 */
class BilingualDynSuffixArray
{
public:
  BilingualDynSuffixArray();
  ~BilingualDynSuffixArray();
  bool Load( const vector<FactorType>& inputFactors,
             const vector<FactorType>& outputTactors,
             string source, string target, string alignments,
             const vector<float> &weight);
  // bool LoadTM( const vector<FactorType>& inputFactors,
  // 	     const vector<FactorType>& outputTactors,
  // 	     string source, string target, string alignments,
  // 	     const vector<float> &weight);
  void GetTargetPhrasesByLexicalWeight(const Phrase& src, vector< pair<Scores, TargetPhrase*> >& target) const;

  void CleanUp(const InputType& source);
  void addSntPair(string& source, string& target, string& alignment);
  pair<float,float>
  GatherCands(Phrase const& src, map<SAPhrase, vector<float> >& pstats) const;

  TargetPhrase*
  GetMosesFactorIDs(const SAPhrase&, const Phrase& sourcePhrase) const;

private:


  mutable WordCoocTable m_wrd_cooc;
  DynSuffixArray * m_srcSA;
  DynSuffixArray * m_trgSA;
  vector<wordID_t>* m_srcCorpus;
  vector<wordID_t>* m_trgCorpus;
  vector<FactorType> m_inputFactors;
  vector<FactorType> m_outputFactors;

  vector<unsigned> m_srcSntBreaks, m_trgSntBreaks;

  Vocab* m_srcVocab, *m_trgVocab;
  ScoresComp* m_scoreCmp;

  vector<SentenceAlignment> m_alignments;
  vector<vector<short> > m_rawAlignments;

  mutable map<pair<wordID_t, wordID_t>, pair<float, float> > m_wordPairCache;
  mutable set<wordID_t> m_freqWordsCached;
  const size_t m_maxPhraseLength, m_maxSampleSize;
  const size_t m_maxPTEntries;
  int LoadCorpus(FactorDirection direction,
                 InputFileStream&, const vector<FactorType>& factors,
                 vector<wordID_t>&, vector<wordID_t>&,
                 Vocab*);
  int LoadAlignments(InputFileStream& aligs);
  int LoadRawAlignments(InputFileStream& aligs);
  int LoadRawAlignments(string& aligs);

  bool ExtractPhrases(const int&, const int&, const int&, vector<PhrasePair*>&, bool=false) const;
  SentenceAlignment GetSentenceAlignment(const int, bool=false) const;
  int SampleSelection(vector<unsigned>&, int = 300) const;

  vector<int> GetSntIndexes(vector<unsigned>&, int, const vector<unsigned>&) const;
  SAPhrase TrgPhraseFromSntIdx(const PhrasePair&) const;
  bool GetLocalVocabIDs(const Phrase&, SAPhrase &) const;
  void CacheWordProbs(wordID_t) const;
  void CacheFreqWords() const;
  void ClearWordInCache(wordID_t);
  pair<float, float> GetLexicalWeight(const PhrasePair&) const;

  int GetSourceSentenceSize(size_t sentenceId) const;
  int GetTargetSentenceSize(size_t sentenceId) const;

};
} // end namespace
#endif
