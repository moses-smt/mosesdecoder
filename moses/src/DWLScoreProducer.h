// $Id$

#ifndef moses_DWLScoreProducer_h
#define moses_DWLScoreProducer_h

#include "FeatureFunction.h"
#include "TypeDef.h"
#include "TranslationOption.h"
#include "ScoreComponentCollection.h"
#include "InputType.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
#include "CeptTable.h"

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/unordered_map.hpp>

namespace Moses
{

template <class KeyT, class ValueT>
class FIFOCache
{
public:
  FIFOCache(size_t maxItems) : m_maxItems(maxItems) {}

  bool HasKey(const KeyT &key)
  {
    return m_predictionCache.find(key) != m_predictionCache.end();
  }

  ValueT &operator[](const KeyT &key)
  {
    if (! HasKey(key)) {
      m_insertions.push(key);
      if (m_insertions.size() > m_maxItems) {
        const KeyT &oldest = m_insertions.front();
        m_predictionCache.erase(oldest);
        m_insertions.pop();
      }
    }    
    return m_predictionCache[key];
  }

private:
  size_t m_maxItems;
  boost::unordered_map<KeyT, ValueT> m_predictionCache;
  std::queue<KeyT> m_insertions;
};

class DWLScoreProducer : public StatelessFeatureFunction
{
public:
  DWLScoreProducer(ScoreIndexManager &scoreIndexManager, const std::vector<float> &weights);

  // read required data files
  bool Initialize(const std::string &modelFile, const std::string &configFile, const std::string &ceptTableFile);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  std::vector<ScoreComponentCollection> ScoreOptions(const std::vector<TranslationOption *> &options, const InputType &src);

  // mandatory methods for Moses feature functions
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  // calculate scores when collecting translation options, not during decoding
  virtual bool ComputeValueInTranslationOption() const
  {
    return true;
  }
private:
  // Load index/vocabulary of target phrases
  bool LoadTargetIndex(const std::string &indexFile);

  // Construct a ScoreComponentCollection with DWL feature set to given score
  ScoreComponentCollection ScoreFactory(float classifierPrediction, float oovCount, float nullCount);

  static void NormalizeSquaredLoss(std::vector<float> &losses);
  static void NormalizeLogisticLossBasic(std::vector<float> &losses);
  static void Normalize2(std::vector<float> &losses);
  static void Normalize3(std::vector<float> &losses);
  static std::string GetSourceCept(const InputType &src, size_t startPos, const std::vector<size_t> &positions);
  static std::vector<std::pair<int, int> > AlignToSpanList(const std::vector<size_t> &positions);

  std::vector<FactorType> m_tgtFactors; // which factors to use; XXX hard-coded for now
  PSD::VWLibraryPredictConsumerFactory *m_consumerFactory;
  PSD::DWLFeatureExtractor *m_extractor;
  PSD::ExtractorConfig m_extractorConfig;
  std::ifstream m_contextFile;
  void (*m_normalizer)(std::vector<float> &); // normalization function
  CeptTable *m_ceptTable;
  FIFOCache<std::string, std::pair<float, float> > m_predictionCache;
  boost::mutex m_cacheLock;
};

}

#endif // moses_DWLScoreProducer_h
