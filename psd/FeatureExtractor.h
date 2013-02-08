#ifndef moses_FeatureExtractor_h
#define moses_FeatureExtractor_h

#include "FeatureConsumer.h"

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>

namespace PSD
{

const size_t FACTOR_FORM = 0; // index of surface forms
const size_t P_E_F_INDEX = 2; // index of P(e|f) score in phrase table

class ExtractorConfig
{
  public:
    ExtractorConfig();
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

    inline bool IsLoaded() const { return m_isLoaded; }

  private:
    // read from configuration
    bool m_paired, m_bagOfWords, m_sourceExternal,
         m_sourceInternal, m_targetInternal, m_mostFrequent,
         m_binnedScores, m_sourceIndicator, m_targetIndicator, 
         m_sourceTargetIndicator, m_STSE,
         m_sourceTopic,
         m_phraseFactor;
    size_t m_windowSize;
    std::vector<size_t> m_factors, m_scoreIndexes;
    std::vector<float> m_scoreBins;

    // internal variables
    bool m_isLoaded;
};

// vector of words, each word is a vector of factors
typedef std::vector<std::vector<std::string> > ContextType; 

typedef std::multimap<size_t, size_t> AlignmentType;

struct TTableEntry
{
  std::string m_id;
  bool m_exists;
  std::vector<float> m_scores;
};

struct Translation
{
  size_t m_index;
  AlignmentType m_alignment;
  std::vector<TTableEntry> m_ttableScores;
};

// index of possible target spans
typedef boost::bimaps::bimap<std::string, size_t> TargetIndexType;

// extract features
class FeatureExtractor
{
public:
  FeatureExtractor(const TargetIndexType &targetIndex, const ExtractorConfig &config, bool train);

  void GenerateFeatures(FeatureConsumer *fc,
    const ContextType &context,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<Translation> &translations,
    std::vector<float> &losses,
    std::string extraFeature = "");

private:
  const TargetIndexType &m_targetIndex;
  const ExtractorConfig &m_config;
  bool m_train;

  std::map<std::string, float> GetMaxProb(const std::vector<Translation> &translations);
  void GenerateContextFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GeneratePhraseFactorFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GenerateInternalFeatures(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateIndicatorFeature(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateConcatIndicatorFeature(const std::vector<std::string> &span1, const std::vector<std::string> &span2, FeatureConsumer *fc);
  void GenerateSTSE(const std::vector<std::string> &span1, const std::vector<std::string> &span2, const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GenerateBagOfWordsFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, size_t factorID, FeatureConsumer *fc);
  void GenerateSourceTopicFeatures(const std::vector<std::string> &wordSpan, const std::vector<std::string> &sourceTopics, FeatureConsumer *fc);
  void GeneratePairedFeatures(const std::vector<std::string> &srcPhrase,
      const std::vector<std::string> &tgtPhrase,
      const AlignmentType &align,
      FeatureConsumer *fc);
  void GenerateScoreFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc);
  void GenerateMostFrequentFeature(const std::vector<TTableEntry> &ttableScores,
      const std::map<std::string, float> &maxProbs,
      FeatureConsumer *fc);
  void GenerateTTableEntryFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc);
  std::string BuildContextFeature(size_t factor, int index, const std::string &value);
};

} // namespace PSD

#endif // moses_FeatureExtractor_h
