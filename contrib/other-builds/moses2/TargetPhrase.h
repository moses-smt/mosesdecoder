/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "Phrase.h"
#include "util/pool.hh"

class Scores;
class Manager;

class TargetPhrase : public Phrase
{
public:
	TargetPhrase(util::Pool *pool, Manager &manager, size_t size);
	virtual ~TargetPhrase();

protected:
	Scores *m_scores;
};

