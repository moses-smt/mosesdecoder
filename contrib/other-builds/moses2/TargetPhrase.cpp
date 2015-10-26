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
#include "System.h"
#include "util/pool.hh"

using namespace std;

TargetPhrase *TargetPhrase::CreateFromString(util::Pool &pool, System &system, const std::string &str)
{
	Moses::FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Moses::Tokenize(str);
	size_t size = toks.size();
	TargetPhrase *ret = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, size);
	ret->Phrase::CreateFromString(vocab, toks);

	return ret;
}

TargetPhrase::TargetPhrase(util::Pool &pool, System &system, size_t size)
:Phrase(pool, size)
{
	m_scores = new (pool.Allocate<Scores>()) Scores(pool, system.GetNumScores());
}

TargetPhrase::~TargetPhrase() {
	// TODO Auto-generated destructor stub
}

