/*
 * TargetPhrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <stdlib.h>
#include "TargetPhrase.h"
#include "Scores.h"
#include "Manager.h"

TargetPhrase::TargetPhrase(util::Pool *pool, Manager &manager, size_t size)
:Phrase(size)
{
	if (pool) {
		m_scores = new (pool->Allocate<Scores>()) Scores(manager.GetNumScores());
	}
	else {
		m_scores = new Scores(manager.GetNumScores());
	}

}

TargetPhrase::~TargetPhrase() {
	// TODO Auto-generated destructor stub
}

