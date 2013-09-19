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

    GapPenaltyProducer(ScoreIndexManager &sci);

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;
    size_t GetNumInputScores() const;

    //Feature function scores rules in hypothesis
    void Evaluate(const TargetPhrase tp, ScoreComponentCollection* out) const;
    bool ComputeValueInTranslationOption() const { return false; };

  };
}//end of namespace

#endif
