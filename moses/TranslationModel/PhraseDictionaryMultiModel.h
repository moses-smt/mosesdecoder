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
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/Util.h"
#include "moses/UserMessage.h"

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

/** Implementation of a virtual phrase table constructed from multiple component phrase tables.
 */
class PhraseDictionaryMultiModel: public PhraseDictionary
{
#ifdef WITH_DLIB
  friend class CrossEntropy;
#endif

public:
  PhraseDictionaryMultiModel(const std::string &line);
  PhraseDictionaryMultiModel(const std::string &description, const std::string &line);
  ~PhraseDictionaryMultiModel();
  void Load();
  virtual void CollectSufficientStatistics(const Phrase& src, std::map<std::string,multiModelStatistics*>* allStats) const;
  virtual TargetPhraseCollection* CreateTargetPhraseCollectionLinearInterpolation(const Phrase& src, std::map<std::string,multiModelStatistics*>* allStats, std::vector<std::vector<float> > &multimodelweights) const;
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
  virtual const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  virtual void InitializeForInput(InputType const&) {
    /* Don't do anything source specific here as this object is shared between threads.*/
  }
  ChartRuleLookupManager *CreateRuleLookupManager(const InputType&, const ChartCellCollectionBase&);
  bool SetParameter(const std::string& key, const std::string& value);

protected:
  std::string m_mode;
  std::vector<std::string> m_pdStr;
  std::vector<PhraseDictionary*> m_pd;
  size_t m_numModels;
  std::vector<float> m_multimodelweights;

  typedef std::vector<TargetPhraseCollection*> PhraseCache;
#ifdef WITH_THREADS
  boost::mutex m_sentenceMutex;
  typedef std::map<boost::thread::id, PhraseCache> SentenceCache;
#else
  typedef PhraseCache SentenceCache;
#endif
  SentenceCache m_sentenceCache;

  PhraseDictionary *FindPhraseDictionary(const std::string &ptName) const;

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

} // end namespace

#endif
