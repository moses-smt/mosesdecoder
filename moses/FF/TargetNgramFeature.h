#ifndef moses_TargetNgramFeature_h
#define moses_TargetNgramFeature_h

#include <string>
#include <map>
#include <boost/unordered_set.hpp>

#include "StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/Word.h"
#include "moses/FactorCollection.h"
#include "moses/LM/SingleFactor.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"

namespace Moses
{

class TargetNgramState : public FFState
{
public:
  TargetNgramState(std::vector<Word> &words): m_words(words) {}
  const std::vector<Word> GetWords() const {
    return m_words;
  }
  virtual int Compare(const FFState& other) const;

private:
  std::vector<Word> m_words;
};

class TargetNgramChartState : public FFState
{
private:
  Phrase m_contextPrefix, m_contextSuffix;

  size_t m_numTargetTerminals; // This isn't really correct except for the surviving hypothesis

  size_t m_startPos, m_endPos, m_inputSize;

  /** Construct the prefix string of up to specified size
   * \param ret prefix string
   * \param size maximum size (typically max lm context window)
   */
  size_t CalcPrefix(const ChartHypothesis &hypo, const int featureId, Phrase &ret, size_t size) const {
    const TargetPhrase &target = hypo.GetCurrTargetPhrase();
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      target.GetAlignNonTerm().GetNonTermIndexMap();

    // loop over the rule that is being applied
    for (size_t pos = 0; pos < target.GetSize(); ++pos) {
      const Word &word = target.GetWord(pos);

      // for non-terminals, retrieve it from underlying hypothesis
      if (word.IsNonTerminal()) {
        size_t nonTermInd = nonTermIndexMap[pos];
        const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
        size = static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureId))->CalcPrefix(*prevHypo, featureId, ret, size);
//        Phrase phrase = static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureId))->GetPrefix();
//        size = phrase.GetSize();
      }
      // for words, add word
      else {
        ret.AddWord(word);
        size--;
      }

      // finish when maximum length reached
      if (size==0)
        break;
    }

    return size;
  }

  /** Construct the suffix phrase of up to specified size
   * will always be called after the construction of prefix phrase
   * \param ret suffix phrase
   * \param size maximum size of suffix
   */
  size_t CalcSuffix(const ChartHypothesis &hypo, int featureId, Phrase &ret, size_t size) const {
    size_t prefixSize = m_contextPrefix.GetSize();
    assert(prefixSize <= m_numTargetTerminals);

    // special handling for small hypotheses
    // does the prefix match the entire hypothesis string? -> just copy prefix
    if (prefixSize == m_numTargetTerminals) {
      size_t maxCount = std::min(prefixSize, size);
      size_t pos= prefixSize - 1;

      for (size_t ind = 0; ind < maxCount; ++ind) {
        const Word &word = m_contextPrefix.GetWord(pos);
        ret.PrependWord(word);
        --pos;
      }

      size -= maxCount;
      return size;
    }
    // construct suffix analogous to prefix
    else {
      const TargetPhrase targetPhrase = hypo.GetCurrTargetPhrase();
      const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
        targetPhrase.GetAlignTerm().GetNonTermIndexMap();
      for (int pos = (int) targetPhrase.GetSize() - 1; pos >= 0 ; --pos) {
        const Word &word = targetPhrase.GetWord(pos);

        if (word.IsNonTerminal()) {
          size_t nonTermInd = nonTermIndexMap[pos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
          size = static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureId))->CalcSuffix(*prevHypo, featureId, ret, size);
        } else {
          ret.PrependWord(word);
          size--;
        }

        if (size==0)
          break;
      }

      return size;
    }
  }

public:
  TargetNgramChartState(const ChartHypothesis &hypo, int featureId, size_t order)
    :m_contextPrefix(order - 1),
     m_contextSuffix(order - 1) {
    m_numTargetTerminals = hypo.GetCurrTargetPhrase().GetNumTerminals();
    const WordsRange range = hypo.GetCurrSourceRange();
    m_startPos = range.GetStartPos();
    m_endPos = range.GetEndPos();
    m_inputSize = hypo.GetManager().GetSource().GetSize();

    const std::vector<const ChartHypothesis*> prevHypos = hypo.GetPrevHypos();
    for (std::vector<const ChartHypothesis*>::const_iterator i = prevHypos.begin(); i != prevHypos.end(); ++i) {
      // keep count of words (= length of generated string)
      m_numTargetTerminals += static_cast<const TargetNgramChartState*>((*i)->GetFFState(featureId))->GetNumTargetTerminals();
    }

    CalcPrefix(hypo, featureId, m_contextPrefix, order - 1);
    CalcSuffix(hypo, featureId, m_contextSuffix, order - 1);
  }

  size_t GetNumTargetTerminals() const {
    return m_numTargetTerminals;
  }

  const Phrase &GetPrefix() const {
    return m_contextPrefix;
  }
  const Phrase &GetSuffix() const {
    return m_contextSuffix;
  }

  int Compare(const FFState& o) const {
    const TargetNgramChartState &other =
      static_cast<const TargetNgramChartState &>( o );

    // prefix
    if (m_startPos > 0) { // not for "<s> ..."
      int ret = GetPrefix().Compare(other.GetPrefix());
      if (ret != 0)
        return ret;
    }

    if (m_endPos < m_inputSize - 1) { // not for "... </s>"
      int ret = GetSuffix().Compare(other.GetSuffix());
      if (ret != 0)
        return ret;
    }
    return 0;
  }
};

/** Sets the features of observed ngrams.
 */
class TargetNgramFeature : public StatefulFeatureFunction
{
public:
  TargetNgramFeature(const std::string &line);

  void Load();

  bool IsUseable(const FactorMask &mask) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual FFState* EvaluateWhenApplied(const Hypothesis& cur_hypo, const FFState* prev_state,
                            ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(const ChartHypothesis& cur_hypo, int featureId,
                                 ScoreComponentCollection* accumulator) const;

  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                , const TranslationOptionList &translationOptionList) const
  {}

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}

  void SetParameter(const std::string& key, const std::string& value);

private:
  FactorType m_factorType;
  Word m_bos;
  boost::unordered_set<std::string> m_vocab;
  size_t m_n;
  bool m_lower_ngrams;
  std::string m_file;

  std::string m_baseName;

  void appendNgram(const Word& word, bool& skip, std::stringstream& ngram) const;
  void MakePrefixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator,
                        size_t numberOfStartPos = 1, size_t offset = 0) const;
  void MakeSuffixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator,
                        size_t numberOfEndPos = 1, size_t offset = 0) const;
};

}

#endif // moses_TargetNgramFeature_h
