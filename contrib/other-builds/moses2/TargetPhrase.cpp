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

namespace Moses2
{

TargetPhrase *TargetPhrase::CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str)
{
	FactorCollection &vocab = system.GetVocab();

	vector<string> toks = Tokenize(str);
	size_t size = toks.size();
	TargetPhrase *ret = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, pt, system, size);
	ret->PhraseImpl::CreateFromString(vocab, system, toks);

	return ret;
}

TargetPhrase::TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size)
:PhraseImpl(pool, size)
,scoreProperties(NULL)
,pt(pt)
{
	m_scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores());

	size_t numWithPtData = system.featureFunctions.GetWithPhraseTableInd().size();
	ffData = new (pool.Allocate<void *>(numWithPtData)) void *[numWithPtData];
}

/*
TargetPhrase::TargetPhrase(MemPool &pool, const System &system, const TargetPhrase &copy)
:PhraseImpl(pool, copy)
,scoreProperties(NULL)
{
	// scores
	m_estimatedScore = copy.m_estimatedScore;
	m_scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores(), copy.GetScores());

	size_t numWithPtData = system.featureFunctions.GetWithPhraseTableInd().size();
	ffData = new (pool.Allocate<void *>(numWithPtData)) void *[numWithPtData];
}
*/

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

SCORE *TargetPhrase::GetScoresProperty(int propertyInd) const
{
	return scoreProperties ? scoreProperties + propertyInd : NULL;
}

}
