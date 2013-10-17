#ifndef moses_DWLFeatureExtractor_h
#define moses_DWLFeatureExtractor_h

#include "FeatureConsumer.h"
#include "FeatureExtractor.h"

#include <vector>
#include <string>
#include <map>
#include <boost/bimap/bimap.hpp>

namespace PSD
{

struct CeptTranslation
{
  size_t m_index;                          // index in the target-phrase vocabulary
  std::vector<float> m_scores;
};

// extract features
class DWLFeatureExtractor
{
public:
  DWLFeatureExtractor(const IndexType &targetIndex, const ExtractorConfig &config, bool train);

  // Generate features for current source phrase and all its translation options, based on 
  // configuration. Calls all auxiliary Generate* methods.
  //
  // In training, reads the &losses parameter and passes them to VW. In prediction, &losses is 
  // an output variable where VW scores are written.
  void GenerateFeatures(FeatureConsumer *fc,
    const ContextType &context,
    const std::vector<std::pair<int, int> > &sourceSpanList,
    const std::vector<CeptTranslation> &translations,
    std::vector<float> &losses);

private:
  const IndexType &m_targetIndex; // Target-phrase vocabulary.
  const ExtractorConfig &m_config;      // Configuration of features.
  bool m_train;                         // Train or predict.

  // Get the highest probability P(e|f) associated with any of the translation options,
  // separately for each phrase table (string keys are phrase-table IDs).
//  std::map<std::string, float> GetMaxProb(const std::vector<CeptTranslation> &translations);

  void GenerateContextFeatures(const ContextType &context, size_t spanStart, size_t spanEnd, FeatureConsumer *fc);
  void GeneratePhraseFactorFeatures(const ContextType &context, const std::vector<std::pair<int, int> > &sourceSpanList, FeatureConsumer *fc);
  void GenerateTargetFactorFeatures(const std::vector<std::string> &targetForms, FeatureConsumer *fc);
  void GenerateInternalFeatures(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateIndicatorFeature(const std::vector<std::string> &span, FeatureConsumer *fc);
  void GenerateGapFeatures(const ContextType &context, const std::vector<std::pair<int, int> > &sourceSpanList, FeatureConsumer *fc);
  void GenerateConcatIndicatorFeature(const std::vector<std::string> &span1, const std::vector<std::string> &span2, FeatureConsumer *fc);
  void GenerateBagOfWordsFeatures(const ContextType &context, int spanStart, int spanEnd, size_t factorID, FeatureConsumer *fc);
  void GeneratePairedFeatures(const std::vector<std::string> &srcPhrase,
      const std::vector<std::string> &tgtPhrase,
      FeatureConsumer *fc);
//  void GenerateScoreFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc);
//  void GenerateMostFrequentFeature(const std::vector<TTableEntry> &ttableScores,
//      const std::map<std::string, float> &maxProbs,
//      FeatureConsumer *fc);
//  void GenerateTTableEntryFeatures(const std::vector<TTableEntry> &ttableScores, FeatureConsumer *fc);
  std::string BuildContextFeature(size_t factor, int index, const std::string &value);
};

} // namespace PSD

#endif // moses_DWLFeatureExtractor_h
