/*
 * TargetPhrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <stdlib.h>
#include "TargetPhrase.h"
#include "Scores.h"
#include "System.h"
#include "MemPool.h"
#include "Search/Manager.h"

using namespace std;

TargetPhrase *TargetPhrase::CreateFromString(MemPool &pool, const System &system, const std::string &str)
{
	FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Tokenize(str);
	size_t size = toks.size();
	TargetPhrase *ret = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, size);
	ret->PhraseImpl::CreateFromString(vocab, system, toks);

	return ret;
}

TargetPhrase::TargetPhrase(MemPool &pool, const System &system, size_t size)
:PhraseImpl(pool, size)
{
	m_scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores());
}

TargetPhrase::~TargetPhrase() {
	// TODO Auto-generated destructor stub
}

SCORE TargetPhrase::GetFutureScore() const
{ return m_scores->GetTotalScore() + m_estimatedScore; }

std::ostream& operator<<(std::ostream &out, const TargetPhrase &obj)
{
	out << (const PhraseImpl&) obj << " SCORES:" << obj.GetScores();
	return out;
}
