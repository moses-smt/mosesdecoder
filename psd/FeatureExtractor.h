#ifndef moses_FeatureExtractor_h
#define moses_FeatureExtractor_h

#include "FeatureConsumer.h"

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>

// vector of words, each word is a vector of factors
typedef std::vector<std::vector<std::string> > ContextType; 

// index of possible target spans
typedef boost::bimaps::bimap<std::string, size_t> TargetIndexType;

// configuration of feature extractor
struct FeatureTypes
{
  bool m_sourceExternal; // generate context features
  bool m_sourceInternal; // generate source-side phrase-internal features
  bool m_targetInternal; // generate target-side phrase-internal features
  bool m_paired;         // generate paired features
  bool m_bagOfWords;     // generate bag-of-words features

  size_t m_contextWindow; // window size for context features

  // list of factors that should be extracted from context (e.g. 0,1,2)
  std::vector<size_t> m_factors;     
};

// extract features
class FeatureExtractor
{
public:
  FeatureExtractor(FeatureTypes ft,                  
    FeatureConsumer *fc,
    const TargetIndexType &targetIndex,
    bool train);

  void GenerateFeatures(const ContextType &context,
    size_t spanStart,
    size_t spanEnd,
    const std::vector<size_t> &translations,
    std::vector<float> &losses);

private:
  FeatureTypes m_ft;
  FeatureConsumer *m_fc;
  const TargetIndexType &m_targetIndex;
  bool m_train;

  void GenerateContextFeatures(const ContextType &context,
    size_t spanStart,
    size_t spanEnd);

  void GenerateInternalFeatures(const std::vector<std::string> &span);
};

#endif // moses_FeatureExtractor_h
