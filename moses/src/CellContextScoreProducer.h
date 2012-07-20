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

    // sets current source-side context
    // must be called when starting to translate a new sentence
    void SetSentence(const InputType &inputSent);

    std::vector<ScoreComponentCollection> ScoreRules(
                                                    size_t startSpan,
                                                    size_t endSpan,
                                                    const std::string &sourceSide,
                                                    std::vector<std::string> *targetRepresentations,
                                                    const InputType &source);

    private :
    PSD::FeatureExtractor *m_extractor;
    PSD::VWLibraryPredictConsumerFactory  *m_consumerFactory;
    PSD::ExtractorConfig m_extractorConfig;
    ScoreComponentCollection ScoreFactory(float score);
    PSD::TargetIndexType m_ruleIndex;
    bool IsOOV(const std::string &targetRep);
    bool LoadRuleIndex(const std::string &indexFile);
    std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now
  };
}//end of namespace

#endif
