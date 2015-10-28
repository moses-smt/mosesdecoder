/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "UnknownWordPenalty.h"
#include "Manager.h"

UnknownWordPenalty::UnknownWordPenalty(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
	// TODO Auto-generated constructor stub

}

UnknownWordPenalty::~UnknownWordPenalty() {
	// TODO Auto-generated destructor stub
}

const TargetPhrases *UnknownWordPenalty::Lookup(const Manager &mgr, InputPath &inputPath) const
{
	TargetPhrases *tps = new TargetPhrases();

	MemPool &pool = mgr.GetPool();
	const System &system = mgr.GetSystem();

	//TargetPhrase *tp = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, 1);
	return NULL;
}
