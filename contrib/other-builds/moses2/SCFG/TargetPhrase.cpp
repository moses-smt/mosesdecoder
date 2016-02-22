/*
 * TargetPhrase.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "TargetPhrase.h"
#include "../legacy/FactorCollection.h"
#include "../legacy/Util2.h"
#include "../System.h"
#include "../Scores.h"
#include "../MemPool.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
TargetPhrase *TargetPhrase::CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str)
{
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


}
}
