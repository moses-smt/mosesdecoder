/*
 * Scores.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "TypeDef.h"

class Scores {
public:
	Scores(size_t numScores);
	virtual ~Scores();

protected:
	SCORE *m_scores;
};

