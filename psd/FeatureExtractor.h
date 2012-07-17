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

// vector of words, each word is a vector of factors
typedef std::vector<std::vector<std::string> > ContextType;

// index of possible target spans
typedef boost::bimaps::bimap<std::string, size_t> TargetIndexType;

class ExtractorConfig
{
  public:
    ExtractorConfig();
    void Load(const std::string &configFile);
    inline bool GetSourceExternal() const { return m_sourceExternal; }
    inline bool GetSourceInternal() const { return m_sourceInternal; }
    inline bool GetTargetInternal() const { return m_targetInternal; }
    inline bool GetSyntaxParent() const { return m_syntaxParent; }
    inline bool GetPaired() const         { return m_paired; }
    inline bool GetBagOfWords() const     { return m_bagOfWords; }
    inline size_t GetWindowSize() const   { return m_windowSize; }
    inline const std::vector<size_t> &GetFactors() const { return m_factors; }

    inline bool IsLoaded() const { return m_isLoaded; }

  private:
    // read from configuration
    bool m_paired, m_bagOfWords, m_sourceExternal,
         m_sourceInternal, m_targetInternal, m_syntaxParent;
    size_t m_windowSize;
    std::vector<size_t> m_factors;

    // internal variables
    bool m_isLoaded;
};

// extract features
class FeatureExtractor
{
public:
  FeatureExtractor(const TargetIndexType &targetIndex, const ExtractorConfig &config, bool train);

    void GenerateFeatures(FeatureConsumer *fc,
    const ContextType &context,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<size_t> &translations,
    std::vector<float> &losses);

    void GenerateFeaturesChart(FeatureConsumer *fc,
    const ContextType &context,
    const std::string &sourceSide,
    std::vector<std::string> &parentLabels,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<size_t> &translations,
    std::vector<float> &losses);

private:
  const TargetIndexType &m_targetIndex;
  const ExtractorConfig &m_config;
  bool m_train;

  void GenerateContextFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GenerateInternalFeatures(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateSyntaxFeatures(const std::vector<std::string> &syntaxLabels, FeatureConsumer *fc);
  std::string BuildContextFeature(size_t factor, int index, const std::string &value);
};

} // namespace PSD
#endif // moses_FeatureExtractor_h
