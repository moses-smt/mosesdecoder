#ifndef moses_TargetNgramFeature_h
#define moses_TargetNgramFeature_h

#include <string>
#include <map>

#include "FactorCollection.h"
#include "FeatureFunction.h"
#include "FFState.h"
#include "Word.h"

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

/** Sets the features of observed ngrams.
 */
class TargetNgramFeature : public StatefulFeatureFunction {
public:
	TargetNgramFeature(FactorType factorType = 0, size_t n = 3, bool lower_ngrams = true):
     StatefulFeatureFunction("dlmn", ScoreProducer::unlimited),
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

	std::string GetScoreProducerWeightShortName(unsigned) const;
	size_t GetNumInputScores() const;

  void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
  float GetSparseProducerWeight() { return m_sparseProducerWeight; }

	virtual const FFState* EmptyHypothesisState(const InputType &input) const;

	virtual FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
	                          ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateChart( const ChartHypothesis& /* cur_hypo */,
                                  int /* featureID */,
                                  ScoreComponentCollection* ) const
                                  {
                                    /* Not implemented */
                                    assert(0);
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
};

}

#endif // moses_TargetNgramFeature_h
