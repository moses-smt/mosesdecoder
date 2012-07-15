#ifndef moses_FeatureExtractor_h
#define moses_FeatureExtractor_h

#include "FeatureConsumer.h"

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>

namespace PSD
{

// vector of words, each word is a vector of factors
typedef std::vector<std::vector<std::string> > ContextType;

// index of possible target spans
typedef boost::bimaps::bimap<std::string, size_t> TargetIndexType;

// configuration of feature extraction, shared, global
const bool PSD_SOURCE_EXTERNAL = true; // generate context features
const bool PSD_SOURCE_INTERNAL = true; // generate source-side phrase-internal features
const bool PSD_TARGET_INTERNAL = true; // generate target-side phrase-internal features
const bool PSD_PAIRED = false;         // generate paired features
const bool PSD_BAG_OF_WORDS = false;   // generate bag-of-words features
const bool SYNTAX_PARENT = true;

const size_t PSD_CONTEXT_WINDOW = 2; // window size for context features

const size_t PSD_FACTORS[3] = { 0, 1, 2 };
const size_t PSD_FACTOR_COUNT = 3;

// extract features
class FeatureExtractor
{
public:
  FeatureExtractor(const TargetIndexType &targetIndex, bool train);

    void GenerateFeatures(FeatureConsumer *fc,
    const ContextType &context,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<size_t> &translations,
    std::vector<float> &losses);

    void GenerateFeaturesChart(FeatureConsumer *fc,
    const ContextType &context,
    std::vector<std::string> sourceSide,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<size_t> &translations,
    std::vector<float> &losses);

private:
  const TargetIndexType &m_targetIndex;
  bool m_train;

  void GenerateContextFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GenerateInternalFeatures(const std::vector<std::string> &span, FeatureConsumer *fc);
  std::string BuildContextFeature(size_t factor, int index, const std::string &value);
};

} // namespace PSD
#endif // moses_FeatureExtractor_h
