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

#ifndef moses_PhraseDictionaryMultiModelCounts_h
#define moses_PhraseDictionaryMultiModelCounts_h

#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#ifndef WIN32
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#endif


#include <boost/unordered_map.hpp>
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/Util.h"
#include "moses/UserMessage.h"
#include <exception>

extern std::vector<std::string> tokenize( const char*);

namespace Moses
{

  typedef boost::unordered_map<std::string, double > lexicalMap;
  typedef boost::unordered_map<std::string, lexicalMap > lexicalMapJoint;
  typedef std::pair<std::vector<float>, std::vector<float> > lexicalPair;
  typedef std::vector<std::vector<lexicalPair> > lexicalCache;

  struct multiModelCountsStatistics : multiModelStatistics {
    std::vector<float> fst, ft;
  };

  struct multiModelCountsStatisticsOptimization: multiModelCountsStatistics {
    std::vector<float> fs;
    lexicalCache lexCachee2f, lexCachef2e;
    size_t f;
  };

  struct lexicalTable {
    lexicalMapJoint joint;
    lexicalMap marginal;
  };

  double InstanceWeighting(std::vector<float> &joint_counts, std::vector<float> &marginals, std::vector<float> &multimodelweights);
  double LinearInterpolationFromCounts(std::vector<float> &joint_counts, std::vector<float> &marginals, std::vector<float> &multimodelweights);


//thrown if alignment information does not match phrase pair (out-of-bound alignment points)
class AlignmentException : public std::runtime_error {
public:
  AlignmentException() : std::runtime_error("AlignmentException") { }
};


/** Implementation of a phrase table with raw counts.
 */
class PhraseDictionaryMultiModelCounts: public PhraseDictionaryMultiModel
{

#ifdef WITH_DLIB
friend class CrossEntropyCounts;
#endif

typedef std::vector< std::set<size_t> > AlignVector;


public:
  PhraseDictionaryMultiModelCounts(size_t m_numScoreComponent, PhraseDictionaryFeature* feature);
  ~PhraseDictionaryMultiModelCounts();
  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::vector<std::string> &files
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , float weightWP);
  TargetPhraseCollection* CreateTargetPhraseCollectionCounts(const Phrase &src, std::vector<float> &fs, std::map<std::string,multiModelCountsStatistics*>* allStats, std::vector<std::vector<float> > &multimodelweights) const;
  void CollectSufficientStatistics(const Phrase &src, std::vector<float> &fs, std::map<std::string,multiModelCountsStatistics*>* allStats) const;
  float GetTargetCount(const Phrase& target, size_t modelIndex) const;
  double GetLexicalProbability( std::string &inner, std::string &outer, const std::vector<lexicalTable*> &tables, std::vector<float> &multimodelweights ) const;
  double ComputeWeightedLexicalTranslation( const Phrase &phraseS, const Phrase &phraseT, AlignVector &alignment, const std::vector<lexicalTable*> &tables, std::vector<float> &multimodelweights, const std::vector<FactorType> &input_factors, const std::vector<FactorType> &output_factors ) const;
  double ComputeWeightedLexicalTranslationFromCache( std::vector<std::vector<std::pair<std::vector<float>, std::vector<float> > > > &cache, std::vector<float> &weights ) const;
  std::pair<PhraseDictionaryMultiModelCounts::AlignVector,PhraseDictionaryMultiModelCounts::AlignVector> GetAlignmentsForLexWeights(const Phrase &phraseS, const Phrase &phraseT, const AlignmentInfo &alignment) const;
  std::vector<std::vector<std::pair<std::vector<float>, std::vector<float> > > > CacheLexicalStatistics( const Phrase &phraseS, const Phrase &phraseT, AlignVector &alignment, const std::vector<lexicalTable*> &tables, const std::vector<FactorType> &input_factors, const std::vector<FactorType> &output_factors );
  void FillLexicalCountsJoint(std::string &wordS, std::string &wordT, std::vector<float> &count, const std::vector<lexicalTable*> &tables) const;
  void FillLexicalCountsMarginal(std::string &wordS, std::vector<float> &count, const std::vector<lexicalTable*> &tables) const;
  void LoadLexicalTable( std::string &fileName, lexicalTable* ltable);
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  void CleanUpComponentModels(const InputType &source);
#ifdef WITH_DLIB
  std::vector<float> MinimizePerplexity(std::vector<std::pair<std::string, std::string> > &phrase_pair_vector);
#endif
  // functions below required by base class
  virtual void InitializeForInput(InputType const&) {
    /* Don't do anything source specific here as this object is shared between threads.*/
  }

private:
  std::vector<PhraseDictionary*> m_inverse_pd;
  std::vector<lexicalTable*> m_lexTable_e2f, m_lexTable_f2e;
  double (*m_combineFunction) (std::vector<float> &joint_counts, std::vector<float> &marginals, std::vector<float> &multimodelweights);
};

#ifdef WITH_DLIB
class CrossEntropyCounts: public OptimizationObjective
{
public:

    CrossEntropyCounts (
        std::vector<multiModelCountsStatisticsOptimization*> &optimizerStats,
        PhraseDictionaryMultiModelCounts * model,
        size_t iFeature
    )
    {
        m_optimizerStats = optimizerStats;
        m_model = model;
        m_iFeature = iFeature;
    }

    double operator() ( const dlib::matrix<double,0,1>& arg) const;

private:
    std::vector<multiModelCountsStatisticsOptimization*> m_optimizerStats;
    PhraseDictionaryMultiModelCounts * m_model;
    size_t m_iFeature;
};
#endif

} // end namespace

#endif