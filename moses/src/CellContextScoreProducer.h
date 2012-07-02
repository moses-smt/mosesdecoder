/* Damt hiero : feature called from inside a chart cell */
#ifndef moses_CellContextScoreProducer_h
#define moses_CellContextScoreProducer_h

#include "vw.h"
#include "ezexample.h"
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


    vector<FactorType> m_srcFactors;
    vector<FactorType> m_tgtFactors;
    CellContextScoreProducer(float weight);

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;
    size_t GetNumInputScores() const;

    // initialize vw
    bool Initialize(const string &modelFile, const string &indexFile);

    // load index between
    bool CellContextScoreProducer::LoadRuleIndex(const string &indexFile)

    vector<ScoreComponentCollection> ScoreOptions(const vector<ChartTranslationOption *> &options,
    const InputType &source);
  };
}//end of namespace

#endif
