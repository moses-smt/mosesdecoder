#pragma once

#include <string>

#include <boost/unordered_set.hpp>

#include <util/string_piece.hh>

#include "StatefulFeatureFunction.h"
#include "FFState.h"

namespace Moses
{

class SparseReorderingState : public FFState
{
public:
	int Compare(const FFState& other) const
	{
		return 0;
	}
};

class SparseReorderingFeature : public StatefulFeatureFunction
{
public:
  enum Type {
    SourceCombined,
    SourceLeft,
    SourceRight
  };

	SparseReorderingFeature(const std::string &line);

	bool IsUseable(const FactorMask &mask) const
		{ return true; }

  void SetParameter(const std::string& key, const std::string& value);

	void Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const
	{}
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{}
	  FFState* Evaluate(
	    const Hypothesis& cur_hypo,
	    const FFState* prev_state,
	    ScoreComponentCollection* accumulator) const
	  {
		  return new SparseReorderingState();
	  }

	  FFState* EvaluateChart(
	    const ChartHypothesis& /* cur_hypo */,
	    int /* featureID - used to index the state in the previous hypotheses */,
	    ScoreComponentCollection* accumulator) const;

	  virtual const FFState* EmptyHypothesisState(const InputType &input) const
	  {
		  return new SparseReorderingState();
	  }

private:

  typedef boost::unordered_set<const Factor*> Vocab;

  void AddNonTerminalPairFeatures(
    const Sentence& sentence, const WordsRange& nt1, const WordsRange& nt2,
      bool isMonotone, ScoreComponentCollection* accumulator) const;

  void LoadVocabulary(const std::string& filename, Vocab& vocab);
  const Factor*  GetFactor(const Word& word, const Vocab& vocab, FactorType factor) const;

  Type m_type;
  FactorType m_sourceFactor;
  FactorType m_targetFactor;
  std::string m_sourceVocabFile;
  std::string m_targetVocabFile;

  const Factor* m_otherFactor;
  
  Vocab m_sourceVocab;
  Vocab m_targetVocab;

};


}

