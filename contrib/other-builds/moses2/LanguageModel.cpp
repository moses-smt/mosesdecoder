/*
 * LanguageModel.cpp
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */
#include <vector>
#include "LanguageModel.h"
#include "System.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "moses/LM/PointerState.h"
#include "moses/Bitmap.h"

using namespace std;

struct LMState : public Moses::PointerState
{
  LMState(MemPool &pool, const Moses::Factor *eos)
  :PointerState(NULL)
  {
	  numWords = 1;
	  lastWords = (const Moses::Factor**) pool.Allocate(sizeof(const Moses::Factor*));
	  lastWords[0] = eos;
  }

  LMState(MemPool &pool, void *lms, const std::vector<const Moses::Factor*> &context)
  :PointerState(lms)
  {
	  numWords = context.size();
	  lastWords = (const Moses::Factor**) pool.Allocate(sizeof(const Moses::Factor*) * numWords);
	  for (size_t i = 0; i < numWords; ++i) {
		  lastWords[i] = context[i];
	  }
  }

  size_t numWords;
  const Moses::Factor** lastWords;
};

////////////////////////////////////////////////////////////////////////////////////////
LanguageModel::LanguageModel(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

LanguageModel::~LanguageModel() {
	// TODO Auto-generated destructor stub
}

void LanguageModel::Load(System &system)
{
  Moses::FactorCollection &fc = system.GetVocab();

  m_sos = fc.AddFactor("<s>", false);
  m_eos = fc.AddFactor("</s>", false);

  Moses::InputFileStream infile(m_path);
  size_t lineNum = 0;
  string line;
  while (getline(infile, line)) {
	  if (++lineNum % 100000 == 0) {
		  cerr << lineNum << " ";
	  }

	  vector<string> substrings;
	  Moses::Tokenize(substrings, line, "\t");

	  if (substrings.size() < 2)
		   continue;

	  assert(substrings.size() == 2 || substrings.size() == 3);

	  SCORE prob = Moses::Scan<SCORE>(substrings[0]);
	  if (substrings[1] == "<unk>") {
		  m_oov = prob;
		  continue;
	  }

	  SCORE backoff = 0.f;
	  if (substrings.size() == 3)
		backoff = Moses::Scan<SCORE>(substrings[2]);

	  // ngram
	  vector<string> key;
	  Moses::Tokenize(key, substrings[1], " ");

	  vector<const Moses::Factor*> factorKey(key.size());
	  for (size_t i = 0; i < key.size(); ++i) {
		  factorKey[factorKey.size() - i - 1] = fc.AddFactor(key[i], false);
	  }

	  m_root.insert(factorKey, LMScores(prob, backoff));
  }

}

void LanguageModel::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Moses::Scan<Moses::FactorType>(value);
  }
  else if (key == "order") {
	  m_order = Moses::Scan<size_t>(value);
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

const Moses::FFState* LanguageModel::EmptyHypothesisState(const Manager &mgr, const Phrase &input) const
{
	MemPool &pool = mgr.GetPool();
	return new (pool.Allocate<LMState>()) LMState(pool, m_eos);
}

void
LanguageModel::EvaluateInIsolation(const System &system,
		  const PhraseBase &source, const TargetPhrase &targetPhrase,
        Scores &scores,
        Scores *estimatedFutureScores) const
{

}

Moses::FFState* LanguageModel::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &scores) const
{
	const LMState &prevLMState = static_cast<const LMState &>(prevState);
	size_t numWords = prevLMState.numWords;

	// context is held backwards
	vector<const Moses::Factor*> context(numWords);
	for (size_t i = 0; i < numWords; ++i) {
		context[i] = prevLMState.lastWords[i];
	}

	SCORE score = 0;
	std::pair<SCORE, void*> fromScoring;
	const Phrase &tp = hypo.GetTargetPhrase();
	for (size_t i = 0; i < tp.GetSize(); ++i) {
		const Word &word = tp[i];
		const Moses::Factor *factor = word[m_factorType];
		ShiftOrPush(context, factor);
		fromScoring = Score(context);
		score += fromScoring.first;

	}

	const Moses::Bitmap &bm = hypo.GetBitmap();
	if (bm.IsComplete()) {
		// everything translated
		ShiftOrPush(context, m_eos);
		fromScoring = Score(context);
		score += fromScoring.first;
		fromScoring.second = NULL;
		context.clear();
	}
	else {
		assert(context.size());
		context.resize(context.size() - 1);
	}

	scores.PlusEquals(mgr.GetSystem(), *this, score);

	// return state

	MemPool &pool = mgr.GetPool();
	return new (pool.Allocate<LMState>()) LMState(pool, fromScoring.second, context);
}

void LanguageModel::ShiftOrPush(std::vector<const Moses::Factor*> &context, const Moses::Factor *factor) const
{
	if (context.size() < m_order) {
		context.resize(context.size() + 1);
	}

	if (context.size()) {
		for (size_t i = 0; i < context.size() - 1; ++i) {
			context[i] = context[i + 1];
		}
	}

	context[0] = factor;
}

std::pair<SCORE, void*> LanguageModel::Score(const std::vector<const Moses::Factor*> &context) const
{
	std::pair<SCORE, void*> ret;
	size_t stoppedAtInd;
	const Node<const Moses::Factor*, LMScores> *node = m_root.getNode(context, stoppedAtInd);

	if (stoppedAtInd == context.size()) {
		// found entire ngram
		ret.first =  node->getValue().prob;
		ret.second = (void*) node;
	}
	else {
		ret.first =  node->getValue().prob;
		ret.second = (void*) node;

		// get backoff score
		//std::vector<const Moses::Factor*> backoff(context.begin() + stoppedAtInd, context.end());
		//BackoffScore(backoff);
	}

	return ret;
}

SCORE LanguageModel::BackoffScore(const std::vector<const Moses::Factor*> &context) const
{
	/*
	SCORE ret;
	size_t stoppedAtInd;
	const Node<const Moses::Factor*, LMScores> *node = m_root.getNode(context, stoppedAtInd);

	if (stoppedAtInd == context.size()) {
		// found entire ngram
		ret =  node->getValue().backoff;
	}
	else {

		// get backoff score
		std::vector<const Moses::Factor*> backoff(context.begin() + stoppedAtInd, context.end());
		BackoffScore(backoff);
	}
	*/
}
