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

	size_t numWords = inputPath.GetRange().GetNumWordsCovered();
	if (numWords > 1) {
		// only create 1 word phrases
		return ret;
	}

	// any other pt translate this?
	const std::vector<TargetPhrases::shared_const_ptr> &allTPS = inputPath.GetTargetPhrases();
	for (size_t i = 0; i < allTPS.size(); ++i) {
		const TargetPhrases::shared_const_ptr &tps = allTPS[i];

		if (tps.get() && tps.get()->GetSize()) {
			return ret;
		}
	}

	const SubPhrase &source = inputPath.GetSubPhrase();
	const Word &sourceWord = source[0];
	const Moses::Factor *factor = sourceWord[0];

	TargetPhrases *tps = new TargetPhrases();

	MemPool &pool = mgr.GetPool();
	const System &system = mgr.GetSystem();

	TargetPhrase *target = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, 1);
	Word &word = (*target)[0];

	//Moses::FactorCollection &fc = system.GetVocab();
	//const Moses::Factor *factor = fc.AddFactor("SSS", false);
	word[0] = factor;

	Scores &scores = target->GetScores();
	scores.PlusEquals(mgr.GetSystem(), *this, -100);

	MemPool &memPool = mgr.GetPool();
	system.GetFeatureFunctions().EvaluateInIsolation(memPool, system, source, *target);

	tps->AddTargetPhrase(*target);
	ret.reset(tps);

	return ret;
}

void
UnknownWordPenalty::EvaluateInIsolation(const System &system,
		const PhraseBase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedFutureScores) const
{

}
