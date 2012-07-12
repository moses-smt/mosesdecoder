/* Damt hiero : feature called from inside a chart cell */
#ifndef moses_CellContextScoreProducer_h
#define moses_CellContextScoreProducer_h

#include "FeatureFunction.h"
#include "TargetPhrase.h"
#include "TypeDef.h"
#include "vw.h"
#include "ScoreComponentCollection.h"
#include <map>
#include <string>
#include <vector>

using namespace std;

namespace Moses {

typedef std::map<std::string, size_t> RuleIndexType;

struct VWInstance
{
  ::vw m_vw;
};

extern VWInstance vwInstance;

class CellContextScoreProducer : public StatelessFeatureFunction
{

 public :

    CellContextScoreProducer(ScoreIndexManager &sci, float weight);

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;
    size_t GetNumInputScores() const;

    vector<string> GetSourceFeatures(const InputType &srcSent,const std::string &sourceSide);
    vector<string> GetTargetFeatures(const std::string &targetRep);


    // initialize vw
    bool Initialize(const string &modelFile, const string &indexFile);

    vector<ScoreComponentCollection> ScoreRules(    const std::string &sourceSide,
                                                    std::vector<std::string> *targetRepresentations,
                                                    const InputType &source);

    private :
        RuleIndexType m_ruleIndex;
        bool IsOOV(const std::string &targetRep);
        bool LoadRuleIndex(const string &indexFile);
        std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now
  };
}//end of namespace

#endif
