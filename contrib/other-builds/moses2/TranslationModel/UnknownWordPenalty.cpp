/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "UnknownWordPenalty.h"
#include "../System.h"
#include "../InputPath.h"
#include "../Scores.h"
#include "../Search/Manager.h"

UnknownWordPenalty::UnknownWordPenalty(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
	ReadParameters();
}

UnknownWordPenalty::~UnknownWordPenalty() {
	// TODO Auto-generated destructor stub
}

void UnknownWordPenalty::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
  BOOST_FOREACH(InputPath &path, inputPaths) {
	  const SubPhrase &phrase = path.subPhrase;

	TargetPhrases::shared_const_ptr tpsPtr;
	tpsPtr = Lookup(mgr, mgr.GetPool(), path);
	path.AddTargetPhrases(*this, tpsPtr);
  }

}

TargetPhrases::shared_const_ptr UnknownWordPenalty::Lookup(const Manager &mgr, MemPool &pool, InputPath &inputPath) const
{
	const System &system = mgr.system;

	TargetPhrases::shared_const_ptr ret;

	size_t numWords = inputPath.range.GetNumWordsCovered();
	if (numWords > 1) {
		// only create 1 word phrases
		return ret;
	}

	// any other pt translate this?
	const std::vector<TargetPhrases::shared_const_ptr> &allTPS = inputPath.targetPhrases;
	for (size_t i = 0; i < allTPS.size(); ++i) {
		const TargetPhrases::shared_const_ptr &tps = allTPS[i];

		if (tps.get() && tps.get()->GetSize()) {
			return ret;
		}
	}

	const SubPhrase &source = inputPath.subPhrase;
	const Word &sourceWord = source[0];
	const Factor *factor = sourceWord[0];

	TargetPhrases *tps = new TargetPhrases(1);

	TargetPhrase *target = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, 1);
	Word &word = (*target)[0];

	//FactorCollection &fc = system.vocab;
	//const Factor *factor = fc.AddFactor("SSS", false);
	word[0] = factor;

	Scores &scores = target->GetScores();
	scores.PlusEquals(mgr.system, *this, -100);

	MemPool &memPool = mgr.GetPool();
	system.featureFunctions.EvaluateInIsolation(memPool, system, source, *target);

	tps->AddTargetPhrase(*target);
	ret.reset(tps);

	return ret;
}

void
UnknownWordPenalty::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScores) const
{

}
