/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_PhraseDictionaryMultiModel_h
#define moses_PhraseDictionaryMultiModel_h

#include "moses/TranslationModel/PhraseDictionary.h"


#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/Util.h"

#ifdef WITH_DLIB
#include <dlib/optimization.h>
#endif

namespace Moses
{

struct multiModelStatistics {
  TargetPhrase *targetPhrase;
  std::vector<std::vector<float> > p;
  ~multiModelStatistics() {
    delete targetPhrase;
  };
};

struct multiModelStatisticsOptimization: multiModelStatistics {
  size_t f;
};

class OptimizationObjective;

struct multiModelPhrase {
  TargetPhrase *targetPhrase;
  std::vector<float> p;
  ~multiModelPhrase() {
    delete targetPhrase;
  };
};

/** Implementation of a virtual phrase table constructed from multiple component phrase tables.
 */
class PhraseDictionaryMultiModel: public PhraseDictionary
{
#ifdef WITH_DLIB
  friend class CrossEntropy;
#endif

public:
  PhraseDictionaryMultiModel(const std::string &line);
  PhraseDictionaryMultiModel(int type, const std::string &line);
  ~PhraseDictionaryMultiModel();
  void Load();
  virtual void CollectSufficientStatistics(const Phrase& src, std::map<std::string,multiModelStatistics*>* allStats) const;
  virtual TargetPhraseCollection* CreateTargetPhraseCollectionLinearInterpolation(const Phrase& src, std::map<std::string,multiModelStatistics*>* allStats, std::vector<std::vector<float> > &multimodelweights) const;
  virtual TargetPhraseCollection* CreateTargetPhraseCollectionAll(const Phrase& src, const bool restricted = false) const;
  std::vector<std::vector<float> > getWeights(size_t numWeights, bool normalize) const;
  std::vector<float> normalizeWeights(std::vector<float> &weights) const;
  void CacheForCleanup(TargetPhraseCollection* tpc);
  void CleanUpAfterSentenceProcessing(const InputType &source);
  virtual void CleanUpComponentModels(const InputType &source);
#ifdef WITH_DLIB
  virtual std::vector<float> MinimizePerplexity(std::vector<std::pair<std::string, std::string> > &phrase_pair_vector);
  std::vector<float> Optimize(OptimizationObjective * ObjectiveFunction, size_t numModels);
#endif
  // functions below required by base class
  virtual const TargetPhraseCollection* GetTargetPhraseCollectionLEGACY(const Phrase& src) const;
  virtual void InitializeForInput(InputType const&) {
    /* Don't do anything source specific here as this object is shared between threads.*/
  }
  ChartRuleLookupManager *CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase&, std::size_t);
  void SetParameter(const std::string& key, const std::string& value);

  const std::vector<float>* GetTemporaryMultiModelWeightsVector() const;
  void SetTemporaryMultiModelWeightsVector(std::vector<float> weights);

protected:
  std::string m_mode;
  std::vector<std::string> m_pdStr;
  std::vector<PhraseDictionary*> m_pd;
  size_t m_numModels;
  std::vector<float> m_multimodelweights;

  typedef std::vector<TargetPhraseCollection*> PhraseCache;
#ifdef WITH_THREADS
  boost::shared_mutex m_lock_cache;
  typedef std::map<boost::thread::id, PhraseCache> SentenceCache;
#else
  typedef PhraseCache SentenceCache;
#endif
  SentenceCache m_sentenceCache;

  PhraseCache& GetPhraseCache() {
#ifdef WITH_THREADS
    {
      // first try read-only lock
      boost::shared_lock<boost::shared_mutex> read_lock(m_lock_cache);
      SentenceCache::iterator i = m_sentenceCache.find(boost::this_thread::get_id());
      if (i != m_sentenceCache.end()) return i->second;
    }
    boost::unique_lock<boost::shared_mutex> lock(m_lock_cache);
    return m_sentenceCache[boost::this_thread::get_id()];
#else
    return m_sentenceCache;
#endif
  }

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_lock_weights;
  std::map<boost::thread::id, std::vector<float> > m_multimodelweights_tmp;
#else
  std::vector<float> m_multimodelweights_tmp;
#endif
};

#ifdef WITH_DLIB
class OptimizationObjective
{
public:

  virtual double operator() ( const dlib::matrix<double,0,1>& arg) const = 0;
};

class CrossEntropy: public OptimizationObjective
{
public:

  CrossEntropy (
    std::vector<multiModelStatisticsOptimization*> &optimizerStats,
    PhraseDictionaryMultiModel * model,
    size_t iFeature
  ) {
    m_optimizerStats = optimizerStats;
    m_model = model;
    m_iFeature = iFeature;
  }

  double operator() ( const dlib::matrix<double,0,1>& arg) const;

protected:
  std::vector<multiModelStatisticsOptimization*> m_optimizerStats;
  PhraseDictionaryMultiModel * m_model;
  size_t m_iFeature;
};
#endif

PhraseDictionary *FindPhraseDictionary(const std::string &ptName);

} // end namespace

#endif
