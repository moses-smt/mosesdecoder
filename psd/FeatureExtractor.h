#ifndef moses_FeatureExtractor_h
#define moses_FeatureExtractor_h

#include "FeatureConsumer.h"

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <exception>
#include <stdexcept>


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
    inline bool GetSourceTargetIndicatorSyntax() const { return m_sourceTargetIndicatorSyntax; }
    inline bool GetSourceTargetIndicatorInternal() const { return m_sourceTargetIndicatorInternal; }
    inline bool GetSourceTargetIndicatorBoW() const { return m_sourceTargetIndicatorBoW; }
    inline bool GetSourceTargetIndicatorPaired() const { return m_sourceTargetIndicatorPaired; }
    inline bool GetSourceTargetIndicatorContext() const { return m_sourceTargetIndicatorContext; }
    inline bool GetSourceTargetIndicatorMostFrequent() const { return m_sourceTargetIndicatorMostFrequent;}
    inline bool GetSourceTargetIndicatorScore() const { return m_sourceTargetIndicatorScore; }
    inline bool GetSyntaxParent() const { return m_syntaxParent; }
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
         m_sourceInternal, m_targetInternal,
         m_syntaxParent, m_mostFrequent,
         m_binnedScores, m_sourceIndicator, m_targetIndicator, m_sourceTargetIndicatorSyntax, m_sourceTargetIndicatorInternal,
         m_sourceTargetIndicatorBoW, m_sourceTargetIndicatorPaired, m_sourceTargetIndicatorContext, m_sourceTargetIndicatorMostFrequent,
         m_sourceTargetIndicatorScore, m_sourceTopic;

    size_t m_windowSize;
    std::vector<size_t> m_factors, m_scoreIndexes;

    std::vector<float> m_scoreBins;

    // internal variables
    bool m_isLoaded;
};

// vector of words, each word is a vector of factors
typedef std::vector<std::vector<std::string> > ContextType;

typedef std::multimap<size_t, size_t> AlignmentType;

struct Translation
{
  size_t m_index;
  AlignmentType m_alignment;
  std::vector<float> m_scores;
};

struct ChartTranslation
{
  size_t m_index;
  AlignmentType m_termAlignment;
  AlignmentType m_nonTermAlignment;
  std::vector<float> m_scores;
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
    std::vector<float> &losses);

  void GenerateFeaturesChartLhs(FeatureConsumer *fc,
    const ContextType &context,
    const std::string &sourceSide,
    const std::vector<std::string> &parentLabels,
    const std::string parent,
    const std::string span,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<ChartTranslation> &translations,
    std::vector<float> &losses);

  void GenerateFeaturesChart(FeatureConsumer *fc,
     const ContextType &context,
     const std::string &sourceSide,
     const std::vector<std::string> &parentLabels,
     const std::string parent,
     const std::string span,
     size_t spanStart,
     size_t spanEnd,
     const std::vector<ChartTranslation> &translations,
     std::vector<float> &losses);

private:
  const TargetIndexType &m_targetIndex;
  const ExtractorConfig &m_config;
  bool m_train;

  float GetMaxProb(const std::vector<Translation> &translations);
  float GetMaxProbChart(const std::vector<ChartTranslation> &translations);
  void GenerateContextFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GenerateInternalFeatures(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateInternalFeaturesChart(const std::vector<std::string> &span, FeatureConsumer *fc, AlignmentType a);
  void GenerateLhsSyntaxFeatures(const std::vector<std::string> &syntaxLabels, const std::string parent,
                              const std::string span, FeatureConsumer *fc);
  //Generate syntax features for each non-terminal
  void GenerateRhsSyntaxFeatures(const std::vector<std::vector<std::string> > &syntaxLabelsPerNonTerm, const std::vector<std::string> parents,
                                const std::vector<std::string> spans, FeatureConsumer *fc);
  void GenerateIndicatorFeature(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateIndicatorFeatureChart(const std::vector<std::string> &span, FeatureConsumer *fc,AlignmentType a);
  void GenerateSourceTargetIndicatorFeatureWithLhsSyntax(
		  const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, FeatureConsumer *fc,AlignmentType nonTermAlign,
		  		const std::vector<std::string> &syntaxLabel, const std::string parent, const std::string span);
  void GenerateSourceTargetIndicatorFeatureWithInternalFeaturesChart(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, FeatureConsumer *fc, AlignmentType nonTermAlign);
  void GenerateSourceTargetIndicatorFeatureWithBagOfWords(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, const ContextType &context, size_t spanStart, size_t spanEnd, size_t factorID, FeatureConsumer *fc, AlignmentType nonTermAlign);
  void GenerateSourceTargetIndicatorFeatureWithPairedFeatures(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, const AlignmentType &alignTerm, const AlignmentType &alignNonTerm, FeatureConsumer *fc);
  void GenerateSourceTargetIndicatorFeatureWithContext(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc, AlignmentType nonTermAlign);
  void GenerateSourceTargetIndicatorMostFrequent(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, FeatureConsumer *fc,AlignmentType nonTermAlign);
  void GenerateSourceTargetIndicatorFeatureWithScoreFeatures(const std::vector<std::string> &sourceSpan, const std::vector<std::string> &targetSpan, const AlignmentType &alignNonTerm, const std::vector<float> scores, FeatureConsumer *fc);
  void GenerateSourceTopicFeatures(const std::vector<std::string> &wordSpan, const std::vector<std::string> &sourceTopics, FeatureConsumer *fc);
  void GenerateBagOfWordsFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, size_t factorID, FeatureConsumer *fc);
  void GeneratePairedFeatures(const std::vector<std::string> &srcPhrase,
      const std::vector<std::string> &tgtPhrase,
      const AlignmentType &align,
      FeatureConsumer *fc);
  void GeneratePairedFeaturesChart(const std::vector<std::string> &srcPhrase,
      const std::vector<std::string> &tgtPhrase,
      const AlignmentType &termAligns,
      const AlignmentType &nonTermAligns,
      FeatureConsumer *fc);
  void GenerateScoreFeatures(const std::vector<float> scores, FeatureConsumer *fc);
  std::string BuildContextFeature(size_t factor, int index, const std::string &value);
};

} // namespace PSD

#endif // moses_FeatureExtractor_h
