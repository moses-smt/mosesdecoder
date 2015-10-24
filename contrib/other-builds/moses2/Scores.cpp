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
#include "moses/Util.h"

using namespace std;

Scores::Scores(util::Pool &pool, size_t numScores)
{
	m_scores = new (pool.Allocate<SCORE>(numScores)) SCORE[numScores];
}

Scores::~Scores() {
	delete m_scores;
}

void Scores::CreateFromString(const std::string &str, const FeatureFunction &featureFunction, const StaticData &staticData)
{
	size_t ffStartInd = featureFunction.GetStartInd();
	vector<SCORE> toks = Moses::Tokenize<SCORE>(str);
	for (size_t i = 0; i < toks.size(); ++i) {
		m_scores[ffStartInd + i] = toks[i];
	}
}
