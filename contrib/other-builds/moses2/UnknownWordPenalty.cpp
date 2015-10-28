/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "UnknownWordPenalty.h"
#include "System.h"
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
	size_t numWords = inputPath.GetRange().GetNumWordsCovered();
	if (numWords > 1) {
		return NULL;
	}

	const SubPhrase &phrase = inputPath.GetSubPhrase();
	const Word &sourceWord = phrase[0];
	const Moses::Factor *factor = sourceWord[0];

	TargetPhrases *tps = new TargetPhrases();

	MemPool &pool = mgr.GetPool();
	const System &system = mgr.GetSystem();

	TargetPhrase *tp = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, 1);
	Word &word = (*tp)[0];

	//Moses::FactorCollection &fc = system.GetVocab();
	//const Moses::Factor *factor = fc.AddFactor("SSS", false);
	word[0] = factor;

	tps->AddTargetPhrase(*tp);

	return tps;
}
