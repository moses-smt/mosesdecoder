#pragma once

#include "moses/FF/FFState.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"

namespace Moses
{

class LanguageModelChartState : public FFState
{
private:
  float m_prefixScore;
  FFState* m_lmRightContext;

  Phrase m_contextPrefix, m_contextSuffix;

  size_t m_numTargetTerminals; // This isn't really correct except for the surviving hypothesis

  const ChartHypothesis &m_hypo;

  /** Construct the prefix string of up to specified size
   * \param ret prefix string
   * \param size maximum size (typically max lm context window)
   */
  size_t CalcPrefix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const {
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
        size = static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID))->CalcPrefix(*prevHypo, featureID, ret, size);
      }
      // for words, add word
      else {
        ret.AddWord(target.GetWord(pos));
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
  size_t CalcSuffix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const {
    UTIL_THROW_IF2(m_contextPrefix.GetSize() > m_numTargetTerminals, "Error");

    // special handling for small hypotheses
    // does the prefix match the entire hypothesis string? -> just copy prefix
    if (m_contextPrefix.GetSize() == m_numTargetTerminals) {
      size_t maxCount = std::min(m_contextPrefix.GetSize(), size);
      size_t pos= m_contextPrefix.GetSize() - 1;

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
      const TargetPhrase& target = hypo.GetCurrTargetPhrase();
      const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
        target.GetAlignNonTerm().GetNonTermIndexMap();
      for (int pos = (int) target.GetSize() - 1; pos >= 0 ; --pos) {
        const Word &word = target.GetWord(pos);

        if (word.IsNonTerminal()) {
          size_t nonTermInd = nonTermIndexMap[pos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
          size = static_cast<const LanguageModelChartState*>(prevHypo->GetFFState(featureID))->CalcSuffix(*prevHypo, featureID, ret, size);
        } else {
          ret.PrependWord(hypo.GetCurrTargetPhrase().GetWord(pos));
          size--;
        }

        if (size==0)
          break;
      }

      return size;
    }
  }


public:
  LanguageModelChartState(const ChartHypothesis &hypo, int featureID, size_t order)
    :m_lmRightContext(NULL)
    ,m_contextPrefix(order - 1)
    ,m_contextSuffix( order - 1)
    ,m_hypo(hypo) {
    m_numTargetTerminals = hypo.GetCurrTargetPhrase().GetNumTerminals();

    for (std::vector<const ChartHypothesis*>::const_iterator i = hypo.GetPrevHypos().begin(); i != hypo.GetPrevHypos().end(); ++i) {
      // keep count of words (= length of generated string)
      m_numTargetTerminals += static_cast<const LanguageModelChartState*>((*i)->GetFFState(featureID))->GetNumTargetTerminals();
    }

    CalcPrefix(hypo, featureID, m_contextPrefix, order - 1);
    CalcSuffix(hypo, featureID, m_contextSuffix, order - 1);
  }

  ~LanguageModelChartState() {
    delete m_lmRightContext;
  }

  void Set(float prefixScore, FFState *rightState) {
    m_prefixScore = prefixScore;
    m_lmRightContext = rightState;
  }

  float GetPrefixScore() const {
    return m_prefixScore;
  }
  FFState* GetRightContext() const {
    return m_lmRightContext;
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

  size_t hash() const {
    size_t ret;

    // prefix
    ret = m_hypo.GetCurrSourceRange().GetStartPos() > 0;
    if (m_hypo.GetCurrSourceRange().GetStartPos() > 0) { // not for "<s> ..."
      size_t hash = hash_value(GetPrefix());
      boost::hash_combine(ret, hash);
    }

    // suffix
    size_t inputSize = m_hypo.GetManager().GetSource().GetSize();
    boost::hash_combine(ret, m_hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1);
    if (m_hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1) { // not for "... </s>"
      size_t hash = m_lmRightContext->hash();
      boost::hash_combine(ret, hash);
    }

    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const LanguageModelChartState &other =
      static_cast<const LanguageModelChartState &>( o );

    // prefix
    if (m_hypo.GetCurrSourceRange().GetStartPos() > 0) { // not for "<s> ..."
      bool ret = GetPrefix() == other.GetPrefix();
      if (ret == false)
        return false;
    }

    // suffix
    size_t inputSize = m_hypo.GetManager().GetSource().GetSize();
    if (m_hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1) { // not for "... </s>"
      bool ret = (*other.GetRightContext()) == (*m_lmRightContext);
      return ret;
    }
    return true;
  }

};

} // namespace

