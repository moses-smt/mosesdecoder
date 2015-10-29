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
	ReadParameters();
}

UnknownWordPenalty::~UnknownWordPenalty() {
	// TODO Auto-generated destructor stub
}

TargetPhrases::shared_const_ptr UnknownWordPenalty::Lookup(const Manager &mgr, InputPath &inputPath) const
{
	TargetPhrases::shared_const_ptr ret;
	TargetPhrases *tps;
	size_t numWords = inputPath.GetRange().GetNumWordsCovered();
	if (numWords > 1) {
		tps = NULL;
		ret.reset(tps);
	}
	else {
		const SubPhrase &source = inputPath.GetSubPhrase();
		const Word &sourceWord = source[0];
		const Moses::Factor *factor = sourceWord[0];

		tps = new TargetPhrases();

		MemPool &pool = mgr.GetPool();
		const System &system = mgr.GetSystem();

		TargetPhrase *target = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, 1);
		Word &word = (*target)[0];

		//Moses::FactorCollection &fc = system.GetVocab();
		//const Moses::Factor *factor = fc.AddFactor("SSS", false);
		word[0] = factor;

		Scores &scores = target->GetScores();
		scores.PlusEquals(mgr.GetSystem(), *this, -100);

		system.GetFeatureFunctions().EvaluateInIsolation(system, source, *target, target->GetScores(), NULL);

		tps->AddTargetPhrase(*target);
		ret.reset(tps);
	}

	return ret;
}

void
UnknownWordPenalty::EvaluateInIsolation(const System &system,
		const PhraseBase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedFutureScores) const
{

}
