/*
 * Scores.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <vector>
#include <cstddef>
#include <stdio.h>
#include "Scores.h"
#include "FeatureFunction.h"
#include "FeatureFunctions.h"
#include "Util.h"
#include "Weights.h"
#include "System.h"
#include "moses/Util.h"

using namespace std;

Scores::Scores(MemPool &pool, size_t numScores)
:m_total(0)
{
	m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
	Init<SCORE>(m_scores, numScores, 0);
}

Scores::Scores(MemPool &pool,
		size_t numScores,
		const Scores &origScores)
:m_total(origScores.m_total)
{
	m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
	memcpy(m_scores, origScores.m_scores, sizeof(SCORE) * numScores);
}

Scores::~Scores() {

}

void Scores::PlusEquals(const System &system,
		const FeatureFunction &featureFunction,
		const std::vector<SCORE> &scores)
{
	assert(scores.size() == featureFunction.GetNumScores());

	const Weights &weights = system.GetWeights();

	size_t ffStartInd = featureFunction.GetStartInd();
	for (size_t i = 0; i < scores.size(); ++i) {
		SCORE incrScore = scores[i];
		m_scores[ffStartInd + i] += incrScore;

		SCORE weight = weights[ffStartInd + i];
		m_total += incrScore * weight;
	}
}

void Scores::PlusEquals(const System &system,
		  const FeatureFunction &featureFunction,
		  const SCORE &score)
{
	assert(featureFunction.GetNumScores() == 1);

	const Weights &weights = system.GetWeights();

	size_t ffStartInd = featureFunction.GetStartInd();
	m_scores[ffStartInd] += score;

	SCORE weight = weights[ffStartInd];
	m_total += score * weight;

}

void Scores::PlusEquals(const System &system, const Scores &scores)
{
	size_t numScores = system.GetFeatureFunctions().GetNumScores();
	for (size_t i = 0; i < numScores; ++i) {
		m_scores[i] = scores.m_scores[i];
	}
	m_total += scores.m_total;

}

void Scores::CreateFromString(const std::string &str, const FeatureFunction &featureFunction, const System &system)
{
	vector<SCORE> scores = Moses::Tokenize<SCORE>(str);
	PlusEquals(system, featureFunction, scores);
}

void Scores::Debug(std::ostream &out, const FeatureFunctions &ffs)
{
	out << m_total << " = ";
	size_t numScores = ffs.GetNumScores();
	for (size_t i = 0; i < numScores; ++i) {
		out << m_scores[i] << " ";
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
