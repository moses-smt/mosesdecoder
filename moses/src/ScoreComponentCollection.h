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
#include <cassert>

#ifdef MPI_ENABLE
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#endif

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
class ScoreComponentCollection {
  friend std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection& rhs);
private:
	FVector m_scores;

public:
  //! Create a new score collection with all values set to 0.0
	ScoreComponentCollection();

  //! Clone a score collection
	ScoreComponentCollection(const ScoreComponentCollection& rhs)
	: m_scores(rhs.m_scores)
	{}

  /** Load from file */
  bool Load(const std::string& filename) 
  {
      return m_scores.load(filename);
  }

  FVector GetScoresVector()
  {
	  return m_scores;
  }

  size_t Size()
  {
	  return m_scores.size();
  }

  //! Set all values to 0.0
	void ZeroAll()
	{
	  m_scores.clear();
	}

	void MultiplyEquals(float scalar);
	void DivideEquals(float scalar);
	void MultiplyEquals(const ScoreComponentCollection& rhs);	

  //! add the score in rhs
	void PlusEquals(const ScoreComponentCollection& rhs)
	{
	  m_scores += rhs.m_scores;
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


	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
	void PlusEquals(const ScoreProducer* sp, const ScoreComponentCollection& scores)
	{
    const std::vector<FName>& names = sp->GetFeatureNames();
    for (std::vector<FName>::const_iterator i = names.begin();
         i != names.end(); ++i) {
      m_scores[*i] += scores.m_scores[*i];
    }
	}

	//! Add scores from a single ScoreProducer only
	//! The length of scores must be equal to the number of score components
	//! produced by sp
  void PlusEquals(const ScoreProducer* sp, const std::vector<float>& scores)
  {
    const std::vector<FName>& names = sp->GetFeatureNames();
    assert(names.size() == scores.size());
    for (size_t i = 0; i < scores.size(); ++i) {
      m_scores[names[i]] += scores[i];
    }
  }

	//! Special version PlusEquals(ScoreProducer, vector<float>)
	//! to add the score from a single ScoreProducer that produces
	//! a single value
	void PlusEquals(const ScoreProducer* sp, float score)
	{
		assert(1 == sp->GetNumScoreComponents());
    m_scores[sp->GetFeatureNames()[0]] += score;
	}

  //For features which have an unbounded number of components
  void PlusEquals(const ScoreProducer*sp, const std::string& name, float score)
  {
    assert(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    m_scores[fname] += score;
  }

	void Assign(const ScoreProducer* sp, const std::vector<float>& scores)
	{
		assert(scores.size() == sp->GetNumScoreComponents());
    const std::vector<FName>& names = sp->GetFeatureNames();
    for (size_t i = 0; i < scores.size(); ++i) {
      m_scores[names[i]] = scores[i];
    }
	}

	//! Special version Assign(ScoreProducer, vector<float>)
	//! to add the score from a single ScoreProducer that produces
	//! a single value
	void Assign(const ScoreProducer* sp, float score)
	{
		assert(1 == sp->GetNumScoreComponents());
    m_scores[sp->GetFeatureNames()[0]] = score;
	}

  //For features which have an unbounded number of components
  void Assign(const ScoreProducer*sp, const std::string name, float score) 
  {
    assert(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    m_scores[fname] = score;
  }


	float InnerProduct(const ScoreComponentCollection& rhs) const
	{
		return m_scores.inner_product(rhs.m_scores);
	}
	
	float PartialInnerProduct(const ScoreProducer* sp, const std::vector<float>& rhs) const
	{
		std::vector<float> lhs = GetScoresForProducer(sp);
		assert(lhs.size() == rhs.size());
		return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0f);
	}

	//! return a vector of all the scores associated with a certain ScoreProducer
	std::vector<float> GetScoresForProducer(const ScoreProducer* sp) const
	{
    size_t components = sp->GetNumScoreComponents();
    if (components == ScoreProducer::unlimited) return std::vector<float>();
    std::vector<float> res(components);
    const std::vector<FName>& names = sp->GetFeatureNames();
    for (size_t i = 0; i < names.size(); ++i) {
      res[i] = m_scores[names[i]];
    }
    return res;
	}

	void ApplyLog(size_t baseOfLog) {
		m_scores.applyLog(baseOfLog);
	}

	void ThresholdScaling(float maxValue)
	{
		// find (smallest) factor for which all weights are <= maxValue
		// 0.1 / 0.14 = 0.714285714
		// 0.1 / 0.17 = 0.588235294
		float factor = 1.0;
		float tmp_factor = 1.0;
		for (size_t i = 0; i < m_scores.size(); ++i) {
			if (m_scores.get(i) > maxValue) {
				tmp_factor = maxValue / m_scores.get(i);
			}
			else if (m_scores.get(i) < maxValue*-1) {
				tmp_factor = maxValue / m_scores.get(i);
				tmp_factor *= -1;
			}

			if (tmp_factor < factor)
				factor = tmp_factor;
		}

		// apply factor
		for (size_t i = 0; i < m_scores.size(); ++i) {
			m_scores.set(i, m_scores.get(i)*factor);
		}
	}

	//! if a ScoreProducer produces a single score (for example, a language model score)
	//! this will return it.  If not, this method will throw
	float GetScoreForProducer(const ScoreProducer* sp) const
	{
    assert(sp->GetNumScoreComponents() == 1);
    return m_scores[sp->GetFeatureNames()[0]];
	}

  //For features which have an unbounded number of components
  float GetScoreForProducer
    (const ScoreProducer* sp, const std::string& name) const
  {
    assert(sp->GetNumScoreComponents() == ScoreProducer::unlimited);
    FName fname(sp->GetScoreProducerDescription(),name);
    return m_scores[fname];
  }

	float GetWeightedScore() const;

  void ZeroAllLM(const LMList& lmList);
  void PlusEqualsAllLM(const LMList& lmList, const ScoreComponentCollection& rhs);
  void L1Normalise();
  float GetL1Norm();
  float GetL2Norm();
  void Save(std::string filename) {m_scores.save(filename);}

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
