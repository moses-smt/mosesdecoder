/*
 * Scores.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <cstddef>
#include "Scores.h"

Scores::Scores(size_t numScores)
{
	m_scores = new SCORE[numScores];

}

Scores::~Scores() {
	delete m_scores;
}

