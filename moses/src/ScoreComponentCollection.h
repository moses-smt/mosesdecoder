// $Id$

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

#ifndef moses_ScoreComponentCollection_h
#define moses_ScoreComponentCollection_h

#include <numeric>
#include <sstream>

#ifdef MPI_ENABLE
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#endif

#include "util/check.hh"

#include "LMList.h"
#include "ScoreProducer.h"
#include "FeatureVector.h"
#include "TypeDef.h"
#include "Util.h"


namespace Moses
{


/*** An unweighted collection of scores for a translation or step in a translation.
 *
 * In the factored phrase-based models that are implemented by moses, there are a set of
 * scores that come from a variety of sources (translation probabilities, language model
 * probablilities, distortion probabilities, generation probabilities).  Furthermore, while
 * some of these scores may be 0, this number is fixed (and generally quite small, ie, less
 * than 15), for a given model.
 *
 * The values contained in ScoreComponentCollection objects are unweighted scores (log-probs).
 *
 * ScoreComponentCollection objects can be added and subtracted, which makes them appropriate
 * to be the datatype used to return the result of a score computations (in this case they will
 * have most values set to zero, except for the ones that are results of the indivudal computation
 * this will then be added into the "running total" in the Hypothesis.  In fact, for a score
 * to be tracked in the hypothesis (and thus to participate in the decoding process), a class
 * representing that score must extend the ScoreProducer abstract base class.  For an example
 * refer to the DistortionScoreProducer class.
 */
class ScoreComponentCollection
{
  friend std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection& rhs);
private:
	FVector m_scores;
  typedef std::pair<size_t,size_t> IndexPair;
  typedef std::map<const ScoreProducer*,IndexPair> ScoreIndexMap;
  static  ScoreIndexMap s_scoreIndexes;
  static size_t s_denseVectorSize;
  static IndexPair GetIndexes(const ScoreProducer* sp) 
  {
    ScoreIndexMap::const_iterator indexIter = s_scoreIndexes.find(sp);
    if (indexIter == s_scoreIndexes.end()) {
      std::cerr << "ERROR: ScoreProducer: " << sp->GetScoreProducerDescription() <<
        " not registered with ScoreIndexMap" << std::endl;
      std::cerr << "You must call ScoreComponentCollection.RegisterScoreProducer() " <<
        " for every ScoreProducer" << std::endl;
      abort();
    }
    return indexIter->second;
  }

public:
  static void ResetCounter() {
    s_denseVectorSize = 0;
  }

  //! Create a new score collection with all values set to 0.0
  ScoreComponentCollection();

  //! Clone a score collection
	ScoreComponentCollection(const ScoreComponentCollection& rhs)
	: m_scores(rhs.m_scores)
	{}

  ScoreComponentCollection& operator=( const ScoreComponentCollection& rhs ) {
    m_scores = rhs.m_scores;
    return *this;
  }

  /**
    * Register a ScoreProducer with a fixed number of scores, so that it can 
    * be allocated space in the dense part of the feature vector.
    **/
  static void RegisterScoreProducer(const ScoreProducer* scoreProducer);
  static void UnregisterScoreProducer(const ScoreProducer* scoreProducer);

  /** Load from file */
  bool Load(const std::string& filename) 
  {
      return m_scores.load(filename);
  }

  const FVector& GetScoresVector() const
  {
	  return m_scores;
  }

  const std::valarray<FValue> &getCoreFeatures() const {
    return m_scores.getCoreFeatures();
  }

  size_t Size() const
  {
	  return m_scores.size();
  }

  void Resize() 
  {
    if (m_scores.coreSize() != s_denseVectorSize) {
      m_scores.resize(s_denseVectorSize);
    }
  }

  /** Create and FVector with the right number of core features */
  static FVector CreateFVector()
  {
    return FVector(s_denseVectorSize);
  }

  void SetToBinaryOf(const ScoreComponentCollection& rhs)
  {
	  m_scores.setToBinaryOf(rhs.m_scores);
  }

  //! Set all values to 0.0
	void ZeroAll()
	{
	  m_scores.clear();
	}

	void MultiplyEquals(float scalar);
	void DivideEquals(float scalar);
	void CoreDivideEquals(float scalar);
	void DivideEquals(const ScoreComponentCollection& rhs);
	void MultiplyEquals(const ScoreComponentCollection& rhs);	
	void MultiplyEqualsBackoff(const ScoreComponentCollection& rhs, float backoff);
	void MultiplyEquals(float core_r0, float sparse_r0);
	void MultiplyEquals(const ScoreProducer* sp, float scalar);

	size_t GetNumberWeights(const ScoreProducer* sp);

	void CoreAssign(const ScoreComponentCollection& rhs)
        {
	  m_scores.coreAssign(rhs.m_scores);
	}

	//! add the score in rhs
	void PlusEquals(const ScoreComponentCollection& rhs)
	{
	  m_scores += rhs.m_scores;
	}

	// add only sparse features
	void SparsePlusEquals(const ScoreComponentCollection& rhs)
	{
	  m_scores.sparsePlusEquals(rhs.m_scores);
	}

	void PlusEquals(const FVector& scores)
	{
		m_scores += scores;
	}

	//! subtract the score in rhs
	void MinusEquals(const ScoreComponentCollection& rhs)
	{
	  m_scores -= rhs.m_scores;
	}

  //For features which have an unbounded number of components
  void MinusEquals(const ScoreProducer*sp, const std::string& name, float score)
  {
    assert(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    m_scores[fname] -= score;
  }

  //For features which have an unbounded number of components
  void SparseMinusEquals(const std::string& full_name, float score)
  {
    FName fname(full_name);
    m_scores[fname] -= score;
  }

	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
	void PlusEquals(const ScoreProducer* sp, const ScoreComponentCollection& scores)
	{
    IndexPair indexes = GetIndexes(sp);
    for (size_t i = indexes.first; i < indexes.second; ++i) {
      m_scores[i] += scores.m_scores[i];
    }
	}

	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
  void PlusEquals(const ScoreProducer* sp, const std::vector<float>& scores)
  {
    IndexPair indexes = GetIndexes(sp);
    CHECK(scores.size() == indexes.second - indexes.first);
    for (size_t i = 0; i < scores.size(); ++i) {
      m_scores[i + indexes.first] += scores[i];
    }
  }

	//! Special version PlusEquals(ScoreProducer, vector<float>)
	//! to add the score from a single ScoreProducer that produces
	//! a single value
	void PlusEquals(const ScoreProducer* sp, float score)
	{
    IndexPair indexes = GetIndexes(sp);
    CHECK(1 == indexes.second - indexes.first);
    m_scores[indexes.first] += score;
	}

  //For features which have an unbounded number of components
  void PlusEquals(const ScoreProducer*sp, const std::string& name, float score)
  {
    CHECK(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    m_scores[fname] += score;
  }

  //For features which have an unbounded number of components
  void SparsePlusEquals(const std::string& full_name, float score)
  {
  	FName fname(full_name);
    m_scores[fname] += score;
  }

  void Assign(const ScoreProducer* sp, const std::vector<float>& scores)
  {
    IndexPair indexes = GetIndexes(sp);
    CHECK(scores.size() == indexes.second - indexes.first);
    for (size_t i = 0; i < scores.size(); ++i) {
      m_scores[i + indexes.first] = scores[i];
    }
  }
  
  //! Special version Assign(ScoreProducer, vector<float>)
  //! to add the score from a single ScoreProducer that produces
  //! a single value
  void Assign(const ScoreProducer* sp, float score)
  {
    IndexPair indexes = GetIndexes(sp);
    CHECK(1 == indexes.second - indexes.first);
    m_scores[indexes.first] = score;
  }
  
  // Assign core weight by index
  void Assign(size_t index, float score) {
    m_scores[index] = score;
  }

  //For features which have an unbounded number of components
  void Assign(const ScoreProducer*sp, const std::string name, float score)
  {
    CHECK(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    m_scores[fname] = score;
  }


  //Read sparse features from string
  void Assign(const ScoreProducer* sp, const std::string line);

  // shortcut: setting the value directly using the feature name
  void Assign(const std::string name, float score)
  {
  	FName fname(name);
  	m_scores[fname] = score;
  }

	float InnerProduct(const ScoreComponentCollection& rhs) const
	{
		return m_scores.inner_product(rhs.m_scores);
	}
	
	float PartialInnerProduct(const ScoreProducer* sp, const std::vector<float>& rhs) const
	{
		std::vector<float> lhs = GetScoresForProducer(sp);
		CHECK(lhs.size() == rhs.size());
		return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0f);
	}

	//! return a vector of all the scores associated with a certain ScoreProducer
	std::vector<float> GetScoresForProducer(const ScoreProducer* sp) const
	{
    size_t components = sp->GetNumScoreComponents();
    if (components == ScoreProducer::unlimited) return std::vector<float>();
    std::vector<float> res(components);
    IndexPair indexes = GetIndexes(sp);
    for (size_t i = 0; i < res.size(); ++i) {
      res[i] = m_scores[i + indexes.first];
    }
    return res;
	}

  //! get subset of scores that belong to a certain sparse ScoreProducer
  FVector GetVectorForProducer(const ScoreProducer* sp) const
  {
    FVector fv(s_denseVectorSize);
    std::string prefix = sp->GetScoreProducerDescription() + FName::SEP;
    for(FVector::FNVmap::const_iterator i = m_scores.cbegin(); i != m_scores.cend(); i++) {
      std::stringstream name;
      name << i->first;
      if (name.str().substr( 0, prefix.length() ).compare( prefix ) == 0)
        fv[i->first] = i->second;
    }
    return fv;
  }

  float GetSparseWeight(const FName& featureName) const
  {
    return m_scores[featureName];
  }
  
  void PrintCoreFeatures() {
    m_scores.printCoreFeatures();
  }

	void ThresholdScaling(float maxValue)
	{
		// find (smallest) factor for which all weights are <= maxValue
		// 0.1 / 0.14 = 0.714285714
		// 0.1 / 0.17 = 0.588235294
    m_scores.thresholdScale(maxValue);
	}

	void CapMax(float maxValue)
	{
		// cap all sparse features to maxValue
		m_scores.capMax(maxValue);
	}

	void CapMin(float minValue)
	{
		// cap all sparse features to minValue
		m_scores.capMin(minValue);
	}

	//! if a ScoreProducer produces a single score (for example, a language model score)
	//! this will return it.  If not, this method will throw
	float GetScoreForProducer(const ScoreProducer* sp) const
	{
    IndexPair indexes = GetIndexes(sp);
    CHECK(indexes.second - indexes.first == 1);
    return m_scores[indexes.first];
	}

  //For features which have an unbounded number of components
  float GetScoreForProducer
    (const ScoreProducer* sp, const std::string& name) const
  {
    CHECK(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    return m_scores[fname];
  }

	float GetWeightedScore() const;

  void ZeroAllLM(const LMList& lmList);
  void PlusEqualsAllLM(const LMList& lmList, const ScoreComponentCollection& rhs);
  void L1Normalise();
  float GetL1Norm() const;
  float GetL2Norm() const;
  float GetLInfNorm() const;
  size_t L1Regularize(float lambda);
  void L2Regularize(float lambda);
  size_t SparseL1Regularize(float lambda);
  void SparseL2Regularize(float lambda);
  void Save(const std::string& filename) const;
  void Save(std::ostream&) const;
  
  void IncrementSparseHopeFeatures() { m_scores.incrementSparseHopeFeatures(); }
  void IncrementSparseFearFeatures() { m_scores.incrementSparseFearFeatures(); }
  void PrintSparseHopeFeatureCounts(std::ofstream& out) { m_scores.printSparseHopeFeatureCounts(out); }
  void PrintSparseFearFeatureCounts(std::ofstream& out) { m_scores.printSparseFearFeatureCounts(out); }
  void PrintSparseHopeFeatureCounts() { m_scores.printSparseHopeFeatureCounts(); }
  void PrintSparseFearFeatureCounts() { m_scores.printSparseFearFeatureCounts(); }
  size_t PruneSparseFeatures(size_t threshold) { return m_scores.pruneSparseFeatures(threshold); }
  size_t PruneZeroWeightFeatures() { return m_scores.pruneZeroWeightFeatures(); }
  void UpdateConfidenceCounts(ScoreComponentCollection &weightUpdate, bool signedCounts) { m_scores.updateConfidenceCounts(weightUpdate.m_scores, signedCounts); }
  void UpdateLearningRates(float decay_core, float decay_sparse, ScoreComponentCollection &confidenceCounts, float core_r0, float sparse_r0) { m_scores.updateLearningRates(decay_core, decay_sparse, confidenceCounts.m_scores, core_r0, sparse_r0); }

#ifdef MPI_ENABLE
  public:
    friend class boost::serialization::access;
		
  private:
    //serialization
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
			ar << m_scores;
    }
		
    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
			ar >> m_scores;

    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
		
#endif

};

struct SCCPlus {
  ScoreComponentCollection operator()
                    (const ScoreComponentCollection& lhs,
                     const ScoreComponentCollection& rhs) {
    ScoreComponentCollection sum(lhs);
    sum.PlusEquals(rhs);
    return sum;
  }
};

}
#endif
