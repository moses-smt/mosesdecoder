/* Damt hiero : feature called from inside a chart cell */
#ifndef moses_GapPenaltyProducer_h
#define moses_GapPenaltyProducer_h

#include "FeatureFunction.h"
#include "TargetPhrase.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"
#include <map>
#include <string>
#include <vector>

namespace Moses {

class GapPenaltyProducer : public StatelessFeatureFunction
{

 public :

	GapPenaltyProducer() : StatelessFeatureFunction("GapPenalty",1) {};

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;
    size_t GetNumInputScores() const;

    //Fabienne Braune : none of these features should be used for the GapPenaltyProducer
    virtual void Evaluate(const PhraseBasedFeatureContext& context,
    											ScoreComponentCollection* accumulator) const
    {
    	//do nothing
    }

    virtual void EvaluateChart(const ChartBasedFeatureContext& context,
                               ScoreComponentCollection* accumulator) const
    {
    	//do nothing
    }

    bool ComputeValueInTranslationOption() const { return false; };

  };
}//end of namespace

#endif
