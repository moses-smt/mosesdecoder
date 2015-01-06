#ifndef moses_ExtractorConfig_h
#define moses_ExtractorConfig_h

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>
#include "IniReader.h"

namespace Discriminative
{

class ExtractorConfig
{
  public:
    void Load(const std::string &configFile);
    inline bool GetSourceExternal() const { return m_sourceExternal; }
    inline bool GetSourceInternal() const { return m_sourceInternal; }
    inline bool GetTargetInternal() const { return m_targetInternal; }
    inline bool GetSourceIndicator() const { return m_sourceIndicator; }
    inline bool GetTargetIndicator() const { return m_targetIndicator; }
    inline bool GetSourceTargetIndicator() const { return m_sourceTargetIndicator; }
    inline bool GetSTSE() const { return m_STSE; }
    inline bool GetPhraseFactor() const   { return m_phraseFactor; }
    inline bool GetPaired() const         { return m_paired; }
    inline bool GetBagOfWords() const     { return m_bagOfWords; }
    inline bool GetMostFrequent() const   { return m_mostFrequent; }
    inline size_t GetWindowSize() const   { return m_windowSize; }
    inline bool GetBinnedScores() const   { return m_binnedScores; }
    inline bool GetSourceTopic() const    { return m_sourceTopic; }
    inline const std::vector<size_t> &GetFactors() const { return m_factors; }
    inline const std::vector<size_t> &GetScoreIndexes() const { return m_scoreIndexes; }
    inline const std::vector<float> &GetScoreBins() const { return m_scoreBins; }
    inline const std::string &GetVWOptionsTrain() const { return m_vwOptsTrain; }
    inline const std::string &GetVWOptionsPredict() const { return m_vwOptsPredict; }
    inline const std::string &GetNormalization() const { return m_normalization; }

    inline bool IsLoaded() const { return m_isLoaded; }

  private:
    // read from configuration
    bool m_paired, m_bagOfWords, m_sourceExternal,
         m_sourceInternal, m_targetInternal, m_mostFrequent,
         m_binnedScores, m_sourceIndicator, m_targetIndicator, 
         m_sourceTargetIndicator, m_STSE, m_sourceTopic, m_phraseFactor;
    std::string m_vwOptsPredict, m_vwOptsTrain, m_normalization;
    size_t m_windowSize;
    std::vector<size_t> m_factors, m_scoreIndexes;
    std::vector<float> m_scoreBins;

    // internal variables
    bool m_isLoaded;
};

} // namespace Discriminative

#endif // moses_ExtractorConfig_h
