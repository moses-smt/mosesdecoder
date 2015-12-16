/*
 * Scores.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <vector>
#include <cstddef>
#include <stdio.h>
#include "Scores.h"
#include "Weights.h"
#include "System.h"
#include "FF/FeatureFunction.h"
#include "FF/FeatureFunctions.h"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

Scores::Scores(const System &system, MemPool &pool, size_t numScores)
:m_total(0)
{
	if (system.nbestSize) {
		m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
		Init<SCORE>(m_scores, numScores, 0);
	}
	else {
		m_scores = NULL;
	}
}

Scores::Scores(MemPool &pool,
		size_t numScores,
		const Scores &origScores)
:m_total(origScores.m_total)
{
	if (origScores.m_scores) {
		m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
		memcpy(m_scores, origScores.m_scores, sizeof(SCORE) * numScores);
	}
	else {
		m_scores = NULL;
	}
}

Scores::~Scores() {

}

void Scores::Reset(size_t numScores)
{
	if (m_scores) {
		Init<SCORE>(m_scores, numScores, 0);
	}
	m_total = 0;
}

void Scores::PlusEquals(const System &system,
		  const FeatureFunction &featureFunction,
		  const SCORE &score)
{
	assert(featureFunction.GetNumScores() == 1);

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();
	if (m_scores) {
		m_scores[ffStartInd] += score;
	}
	SCORE weight = weights[ffStartInd];
	m_total += score * weight;

}

void Scores::PlusEquals(const System &system,
		const FeatureFunction &featureFunction,
		const std::vector<SCORE> &scores)
{
	assert(scores.size() == featureFunction.GetNumScores());

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();
	for (size_t i = 0; i < scores.size(); ++i) {
		SCORE incrScore = scores[i];
		if (m_scores) {
			m_scores[ffStartInd + i] += incrScore;
		}
		//cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
		SCORE weight = weights[ffStartInd + i];
		m_total += incrScore * weight;
	}
}

void Scores::PlusEquals(const System &system,
		const FeatureFunction &featureFunction,
		const Vector<SCORE> &scores)
{
	assert(scores.size() == featureFunction.GetNumScores());

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();
	for (size_t i = 0; i < scores.size(); ++i) {
		SCORE incrScore = scores[i];
		if (m_scores) {
			m_scores[ffStartInd + i] += incrScore;
		}
		//cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
		SCORE weight = weights[ffStartInd + i];
		m_total += incrScore * weight;
	}
}

void Scores::PlusEquals(const System &system,
		const FeatureFunction &featureFunction,
		SCORE scores[])
{
	//assert(scores.size() == featureFunction.GetNumScores());

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();
	for (size_t i = 0; i < featureFunction.GetNumScores(); ++i) {
		SCORE incrScore = scores[i];
		if (m_scores) {
			m_scores[ffStartInd + i] += incrScore;
		}
		//cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
		SCORE weight = weights[ffStartInd + i];
		m_total += incrScore * weight;
	}
}

void Scores::PlusEquals(const System &system, const Scores &other)
{
	size_t numScores = system.featureFunctions.GetNumScores();
	if (m_scores) {
		for (size_t i = 0; i < numScores; ++i) {
			m_scores[i] += other.m_scores[i];
		}
	}
	m_total += other.m_total;

}

void Scores::Assign(const System &system,
		  const FeatureFunction &featureFunction,
		  const SCORE &score)
{
	assert(featureFunction.GetNumScores() == 1);

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();

	if (m_scores) {
		assert(m_scores[ffStartInd] == 0);
		m_scores[ffStartInd] = score;
	}
	SCORE weight = weights[ffStartInd];
	m_total += score * weight;

}

void Scores::Assign(const System &system,
		  const FeatureFunction &featureFunction,
		  const std::vector<SCORE> &scores)
{
	assert(scores.size() == featureFunction.GetNumScores());

	const Weights &weights = system.weights;

	size_t ffStartInd = featureFunction.GetStartInd();
	for (size_t i = 0; i < scores.size(); ++i) {
		SCORE incrScore = scores[i];

		if (m_scores) {
			assert(m_scores[ffStartInd + i] == 0);
			m_scores[ffStartInd + i] = incrScore;
		}
		//cerr << "ffStartInd=" << ffStartInd << " " << i << endl;
		SCORE weight = weights[ffStartInd + i];
		m_total += incrScore * weight;
	}
}

void Scores::CreateFromString(const std::string &str,
		const FeatureFunction &featureFunction,
		const System &system,
		bool transformScores)
{
	vector<SCORE> scores = Tokenize<SCORE>(str);
	if (transformScores) {
	    std::transform(scores.begin(), scores.end(), scores.begin(), TransformScore);
	}

	/*
	std::copy(scores.begin(),scores.end(),
	              std::ostream_iterator<SCORE>(cerr," "));
	*/

	PlusEquals(system, featureFunction, scores);
}

void Scores::Debug(std::ostream &out, const FeatureFunctions &ffs) const
{
	out << m_total << " = ";
	if (m_scores) {
	  BOOST_FOREACH(const FeatureFunction *ff, ffs.GetFeatureFunctions()) {
		  out << ff->GetName() << ":";
		  for (size_t i = ff->GetStartInd(); i < (ff->GetStartInd() + ff->GetNumScores()); ++i) {
			out << m_scores[i] << " ";
		  }

	  }

		size_t numScores = ffs.GetNumScores();
		for (size_t i = 0; i < numScores; ++i) {
			out << m_scores[i] << " ";
		}
	}
}

std::ostream& operator<<(std::ostream &out, const Scores &obj)
{
	out << obj.m_total;

	if (obj.m_scores) {
		// don't know num of scores
	}
	return out;
}

}

