#ifndef moses_TargetNgramFeature_h
#define moses_TargetNgramFeature_h

#include <string>
#include <map>

#include "FactorCollection.h"
#include "FeatureFunction.h"
#include "FFState.h"
#include "Word.h"

#include "LM/SingleFactor.h"
#include "ChartHypothesis.h"
#include "ChartManager.h"

namespace Moses
{

class TargetNgramState : public FFState {
  public:
		TargetNgramState(std::vector<Word> &words): m_words(words) {}
		const std::vector<Word> GetWords() const {return m_words;}
    virtual int Compare(const FFState& other) const;

  private:
    std::vector<Word> m_words;
};

class TargetNgramChartState : public FFState
{
private:
  FFState* m_lmRightContext;

  Phrase m_contextPrefix, m_contextSuffix;

  size_t m_numTargetTerminals; // This isn't really correct except for the surviving hypothesis

  const ChartHypothesis &m_hypo;

  /** Construct the prefix string of up to specified size
   * \param ret prefix string
   * \param size maximum size (typically max lm context window)
   */
  size_t CalcPrefix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const
  {
    const TargetPhrase &target = hypo.GetCurrTargetPhrase();
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      target.GetAlignmentInfo().GetNonTermIndexMap();

    // loop over the rule that is being applied
    for (size_t pos = 0; pos < target.GetSize(); ++pos) {
      const Word &word = target.GetWord(pos);

      // for non-terminals, retrieve it from underlying hypothesis
      if (word.IsNonTerminal()) {
        size_t nonTermInd = nonTermIndexMap[pos];
        const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
        size = static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureID))->CalcPrefix(*prevHypo, featureID, ret, size);
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
  size_t CalcSuffix(const ChartHypothesis &hypo, int featureID, Phrase &ret, size_t size) const
  {
    assert(m_contextPrefix.GetSize() <= m_numTargetTerminals);

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
      const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
        hypo.GetCurrTargetPhrase().GetAlignmentInfo().GetNonTermIndexMap();
      for (int pos = (int) hypo.GetCurrTargetPhrase().GetSize() - 1; pos >= 0 ; --pos) {
        const Word &word = hypo.GetCurrTargetPhrase().GetWord(pos);

        if (word.IsNonTerminal()) {
          size_t nonTermInd = nonTermIndexMap[pos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermInd);
          size = static_cast<const TargetNgramChartState*>(prevHypo->GetFFState(featureID))->CalcSuffix(*prevHypo, featureID, ret, size);
        }
        else {
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
  TargetNgramChartState(const ChartHypothesis &hypo, int featureID, size_t order)
      :m_lmRightContext(NULL)
      ,m_contextPrefix(Output, order - 1)
      ,m_contextSuffix(Output, order - 1)
      ,m_hypo(hypo)
  {
    m_numTargetTerminals = hypo.GetCurrTargetPhrase().GetNumTerminals();

    for (std::vector<const ChartHypothesis*>::const_iterator i = hypo.GetPrevHypos().begin(); i != hypo.GetPrevHypos().end(); ++i) {
      // keep count of words (= length of generated string)
      m_numTargetTerminals += static_cast<const TargetNgramChartState*>((*i)->GetFFState(featureID))->GetNumTargetTerminals();
    }

    CalcPrefix(hypo, featureID, m_contextPrefix, order - 1);
    CalcSuffix(hypo, featureID, m_contextSuffix, order - 1);
  }

  ~TargetNgramChartState() {
    delete m_lmRightContext;
  }

  void Set(FFState *rightState) {
    m_lmRightContext = rightState;
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

  int Compare(const FFState& o) const {
    const TargetNgramChartState &other =
      dynamic_cast<const TargetNgramChartState &>( o );

    // prefix
    if (m_hypo.GetCurrSourceRange().GetStartPos() > 0) // not for "<s> ..."
    {
      int ret = GetPrefix().Compare(other.GetPrefix());
      if (ret != 0)
        return ret;
    }

    // suffix
    size_t inputSize = m_hypo.GetManager().GetSource().GetSize();
    if (m_hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1)// not for "... </s>"
    {
      int ret = other.GetRightContext()->Compare(*m_lmRightContext);
      if (ret != 0)
        return ret;
    }
    return 0;
  }
};

/** Sets the features of observed ngrams.
 */
class TargetNgramFeature : public StatefulFeatureFunction, public LanguageModelPointerState {
public:
	TargetNgramFeature(FactorType factorType = 0, size_t n = 3, bool lower_ngrams = true):
     StatefulFeatureFunction("dlm", ScoreProducer::unlimited),
     m_factorType(factorType),
     m_n(n),
     m_lower_ngrams(lower_ngrams),
     m_sparseProducerWeight(1)
  {
    FactorCollection& factorCollection = FactorCollection::Instance();
    const Factor* bosFactor =
       factorCollection.AddFactor(Output,m_factorType,BOS_);
    m_bos.SetFactor(m_factorType,bosFactor);
  }

	bool Load(const std::string &filePath);
	bool Load(const std::string&, Moses::FactorType, size_t);

	std::string GetScoreProducerWeightShortName(unsigned) const;
	size_t GetNumInputScores() const;

  void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
  float GetSparseProducerWeight() const { return m_sparseProducerWeight; }

	virtual const FFState* EmptyHypothesisState(const InputType &input) const;

	virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
	                          ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart(const ChartHypothesis& cur_hypo, int featureID,
                                  ScoreComponentCollection* accumulator) const;

  LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;

  size_t GetNGramOrder() const {
		return m_n;
	}

private:
  FactorType m_factorType;
  Word m_bos;
	std::set<std::string> m_vocab;
	size_t m_n;
	bool m_lower_ngrams;

	// additional weight that all sparse weights are scaled with
	float m_sparseProducerWeight;

	void appendNgram(const Word& word, bool& skip, std::string& ngram) const;
	void ShiftOrPush(std::vector<const Word*> &contextFactor, const Word &word) const;
	void MakePrefixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator,
			      size_t numberOfStartPos = 1, size_t offset = 0) const;
	void MakeSuffixNgrams(std::vector<const Word*> &contextFactor, ScoreComponentCollection* accumulator,
			      size_t numberOfEndPos = 1, size_t offset = 0) const;

	std::vector<FactorType> GetFactorType() const {
		std::vector<FactorType> factorType;
		factorType.push_back(m_factorType);
		return factorType;
	}
};

}

#endif // moses_TargetNgramFeature_h
