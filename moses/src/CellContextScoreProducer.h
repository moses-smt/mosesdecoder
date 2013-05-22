/* Damt hiero : feature called from inside a chart cell */
#ifndef moses_CellContextScoreProducer_h
#define moses_CellContextScoreProducer_h

#include "FeatureFunction.h"
#include "TargetPhrase.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
#include <map>
#include <string>
#include <vector>

namespace Moses {

class CellContextScoreProducer : public StatelessFeatureFunction
{

 public :

    CellContextScoreProducer(ScoreIndexManager &sci, float weight);
    ~CellContextScoreProducer();

    // mandatory methods for features
    std::string GetScoreProducerDescription(unsigned) const;
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumScoreComponents() const;
    size_t GetNumInputScores() const;


    // initialize vw
    bool Initialize(const std::string &modelFile, const std::string &indexFile, const std::string &configFile);


    std::vector<ScoreComponentCollection> ScoreRules(
                                                    size_t startSpan,
                                                    size_t endSpan,
                                                    const std::string &sourceSide,
                                                    std::vector<std::string> *targetRepresentations,
                                                    const InputType &source,
                                                    std::map<std::string,TargetPhrase*> * targetMap);


    void CheckIndex(const std::string &targetRep);
    PSD::ChartTranslation GetPSDTranslation(const std::string targetRep, const TargetPhrase * tp);
    virtual bool ComputeValueInTranslationOption() const { return true; }
    void Normalize(std::vector<float> &losses);
    void Normalize0(std::vector<float> &losses);
    void Normalize1(std::vector<float> &losses);
    double LogAddition(double logA, double logB, double logAddPrecision);
    void Interpolate(std::vector<float> &losses, std::vector<float> &pEgivenF, float interpolParam);

    private :
    PSD::TargetIndexType m_ruleIndex; //FB : this target index type remains empty during decoding
    PSD::FeatureExtractor *m_extractor, *m_debugExtractor;
    PSD::VWLibraryPredictConsumerFactory  *m_consumerFactory;
    PSD::VWFileTrainConsumer      *m_debugConsumer;
    PSD::ExtractorConfig m_extractorConfig;
    bool IsOOV(const std::string &targetRep);
    bool LoadRuleIndex(const std::string &indexFile);
    std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now

    public :
    ScoreComponentCollection ScoreFactory(float score);

  };
}//end of namespace

#endif
