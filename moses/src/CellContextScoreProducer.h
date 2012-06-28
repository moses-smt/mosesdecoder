/* Damt hiero : feature called from inside a chart cell */
#ifndef moses_CellContextScoreProducer_h
#define moses_CellContextScoreProducer_h

#include "FeatureFunction.h"
#include "TargetPhrase.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"
#include <map>
#include <string>
#include <vector>

using namespace std;

namespace Moses {

  class CellContextScoreProducer : public StatelessFeatureFunction
{

 public :
  CellContextScoreProducer(float weight,string ttableFile);

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;

    // load rule table
    void LoadScores(const std::string& );

    // score target phrase
    void EvaluateChart( const TargetPhrase& , ScoreComponentCollection*, float score ) const;

  };
}//end of namespace

#endif
