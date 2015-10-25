/*
 * Scores.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <vector>
#include <cstddef>
#include "Scores.h"
#include "FeatureFunction.h"
#include "Util.h"
#include "Weights.h"
#include "System.h"
#include "moses/Util.h"

using namespace std;

Scores::Scores(util::Pool &pool, size_t numScores)
:m_total(0)
{
	m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
	Init<SCORE>(m_scores, numScores, 0);
}

Scores::~Scores() {
	delete m_scores;
}

void Scores::PlusEquals(const std::vector<SCORE> &scores, const FeatureFunction &featureFunction, const System &system)
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

void Scores::CreateFromString(const std::string &str, const FeatureFunction &featureFunction, const System &system)
{
	vector<SCORE> scores = Moses::Tokenize<SCORE>(str);
	PlusEquals(scores, featureFunction, system);
}
