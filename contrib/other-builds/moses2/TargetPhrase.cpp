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
#include "MemPool.h"

using namespace std;

TargetPhrase *TargetPhrase::CreateFromString(MemPool &pool, const System &system, const std::string &str)
{
	Moses::FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Moses::Tokenize(str);
	size_t size = toks.size();
	TargetPhrase *ret = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, size);
	ret->Phrase::CreateFromString(vocab, toks);

	return ret;
}

TargetPhrase::TargetPhrase(MemPool &pool, const System &system, size_t size)
:Phrase(pool, size)
{
	m_scores = new (pool.Allocate<Scores>()) Scores(pool, system.GetFeatureFunctions().GetNumScores());
}

TargetPhrase::~TargetPhrase() {
	// TODO Auto-generated destructor stub
}

SCORE TargetPhrase::GetFutureScore() const
{ return m_scores->GetTotalScore() + m_estimatedScore; }

std::ostream& operator<<(std::ostream &out, const TargetPhrase &obj)
{
	out << (const Phrase&) obj << " SCORES:" << obj.GetScores();
	return out;
}
