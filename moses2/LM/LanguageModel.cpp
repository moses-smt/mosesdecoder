/*
 * LanguageModel.cpp
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */
#include <vector>
#include "LanguageModel.h"
#include "../Phrase.h"
#include "../System.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "../FF/PointerState.h"
#include "../legacy/Util2.h"
#include "../legacy/InputFileStream.h"
#include "../legacy/Bitmap.h"
#include "../legacy/Util2.h"

using namespace std;

namespace Moses2
{

struct LMState: public PointerState {
  LMState() :
    PointerState() {
    // uninitialised
  }

  void Set(MemPool &pool, void *lms, const std::vector<const Factor*> &context) {
    lmstate = lms;

    numWords = context.size();
    lastWords = (const Factor**) pool.Allocate(
                  sizeof(const Factor*) * numWords);
    for (size_t i = 0; i < numWords; ++i) {
      lastWords[i] = context[i];
    }
  }

  void Init(MemPool &pool, const Factor *factor) {
    lmstate = NULL;
    numWords = 1;
    lastWords = (const Factor**) pool.Allocate(sizeof(const Factor*));
    lastWords[0] = factor;
  }

  size_t numWords;
  const Factor** lastWords;
};

////////////////////////////////////////////////////////////////////////////////////////
LanguageModel::LanguageModel(size_t startInd, const std::string &line) :
  StatefulFeatureFunction(startInd, line), m_oov(-100)
{
  ReadParameters();
}

LanguageModel::~LanguageModel()
{
  // TODO Auto-generated destructor stub
}

void LanguageModel::Load(System &system)
{
  FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor(BOS_, system, false);
  m_eos = fc.AddFactor(EOS_, system, false);

  InputFileStream infile(m_path);
  size_t lineNum = 0;
  string line;
  while (getline(infile, line)) {
    if (++lineNum % 100000 == 0) {
      cerr << lineNum << " ";
    }

    vector<string> substrings = Tokenize(line, "\t");

    if (substrings.size() < 2) continue;

    assert(substrings.size() == 2 || substrings.size() == 3);

    SCORE prob = TransformLMScore(Scan<SCORE>(substrings[0]));
    if (substrings[1] == "<unk>") {
      m_oov = prob;
      continue;
    }

    SCORE backoff = 0.f;
    if (substrings.size() == 3) {
      backoff = TransformLMScore(Scan<SCORE>(substrings[2]));
    }

    // ngram
    vector<string> key = Tokenize(substrings[1], " ");

    vector<const Factor*> factorKey(key.size());
    for (size_t i = 0; i < key.size(); ++i) {
      factorKey[factorKey.size() - i - 1] = fc.AddFactor(key[i], system, false);
    }

    m_root.insert(factorKey, LMScores(prob, backoff));
  }

}

void LanguageModel::SetParameter(const std::string& key,
                                 const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "order") {
    m_order = Scan<size_t>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* LanguageModel::BlankState(MemPool &pool, const System &sys) const
{
  return new (pool.Allocate<LMState>()) LMState();
}

void LanguageModel::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
    const InputType &input, const Hypothesis &hypo) const
{
  LMState &stateCast = static_cast<LMState&>(state);

  MemPool &pool = mgr.GetPool();
  stateCast.Init(pool, m_bos);
}

void LanguageModel::EvaluateInIsolation(MemPool &pool, const System &system,
                                        const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
                                        SCORE &estimatedScore) const
{
  if (targetPhrase.GetSize() == 0) {
    return;
  }

  SCORE score = 0;
  SCORE nonFullScore = 0;
  vector<const Factor*> context;
//	context.push_back(m_bos);

  context.reserve(m_order);
  for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
    const Factor *factor = targetPhrase[i][m_factorType];
    ShiftOrPush(context, factor);

    if (context.size() == m_order) {
      std::pair<SCORE, void*> fromScoring = Score(context);
      score += fromScoring.first;
    } else {
      std::pair<SCORE, void*> fromScoring = Score(context);
      nonFullScore += fromScoring.first;
    }
  }

  scores.PlusEquals(system, *this, score);
  SCORE weightedScore = Scores::CalcWeightedScore(system, *this, nonFullScore);
  estimatedScore += weightedScore;
}

void LanguageModel::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                        const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                        SCORE &estimatedScore) const
{
}

void LanguageModel::EvaluateWhenApplied(const ManagerBase &mgr,
                                        const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                        FFState &state) const
{
  const LMState &prevLMState = static_cast<const LMState &>(prevState);
  size_t numWords = prevLMState.numWords;

  // context is held backwards
  vector<const Factor*> context(numWords);
  for (size_t i = 0; i < numWords; ++i) {
    context[i] = prevLMState.lastWords[i];
  }
  //DebugContext(context);

  SCORE score = 0;
  std::pair<SCORE, void*> fromScoring;
  const TargetPhrase<Moses2::Word> &tp = hypo.GetTargetPhrase();
  for (size_t i = 0; i < tp.GetSize(); ++i) {
    const Word &word = tp[i];
    const Factor *factor = word[m_factorType];
    ShiftOrPush(context, factor);
    fromScoring = Score(context);
    score += fromScoring.first;
  }

  const Bitmap &bm = hypo.GetBitmap();
  if (bm.IsComplete()) {
    // everything translated
    ShiftOrPush(context, m_eos);
    fromScoring = Score(context);
    score += fromScoring.first;
    fromScoring.second = NULL;
    context.clear();
  } else {
    assert(context.size());
    if (context.size() == m_order) {
      context.resize(context.size() - 1);
    }
  }

  scores.PlusEquals(mgr.system, *this, score);

  // return state
  //DebugContext(context);

  LMState &stateCast = static_cast<LMState&>(state);
  MemPool &pool = mgr.GetPool();
  stateCast.Set(pool, fromScoring.second, context);
}

void LanguageModel::ShiftOrPush(std::vector<const Factor*> &context,
                                const Factor *factor) const
{
  if (context.size() < m_order) {
    context.resize(context.size() + 1);
  }
  assert(context.size());

  for (size_t i = context.size() - 1; i > 0; --i) {
    context[i] = context[i - 1];
  }

  context[0] = factor;
}

std::pair<SCORE, void*> LanguageModel::Score(
  const std::vector<const Factor*> &context) const
{
  //cerr << "context=";
  //DebugContext(context);

  std::pair<SCORE, void*> ret;

  typedef Node<const Factor*, LMScores> LMNode;
  const LMNode *node = m_root.getNode(context);
  if (node) {
    ret.first = node->getValue().prob;
    ret.second = (void*) node;
  } else {
    SCORE backoff = 0;
    std::vector<const Factor*> backOffContext(context.begin() + 1,
        context.end());
    node = m_root.getNode(backOffContext);
    if (node) {
      backoff = node->getValue().backoff;
    }

    std::vector<const Factor*> newContext(context.begin(), context.end() - 1);
    std::pair<SCORE, void*> newRet = Score(newContext);

    ret.first = backoff + newRet.first;
    ret.second = newRet.second;
  }

  //cerr << "score=" << ret.first << endl;
  return ret;
}

SCORE LanguageModel::BackoffScore(
  const std::vector<const Factor*> &context) const
{
  //cerr << "backoff=";
  //DebugContext(context);

  SCORE ret;
  size_t stoppedAtInd;
  const Node<const Factor*, LMScores> &node = m_root.getNode(context,
      stoppedAtInd);

  if (stoppedAtInd == context.size()) {
    // found entire ngram
    ret = node.getValue().backoff;
  } else {
    if (stoppedAtInd == 0) {
      ret = m_oov;
      stoppedAtInd = 1;
    } else {
      ret = node.getValue().backoff;
    }

    // recursive
    std::vector<const Factor*> backoff(context.begin() + stoppedAtInd,
                                       context.end());
    ret += BackoffScore(backoff);
  }

  return ret;
}

void LanguageModel::DebugContext(
  const std::vector<const Factor*> &context) const
{
  for (size_t i = 0; i < context.size(); ++i) {
    cerr << context[i]->GetString() << " ";
  }
  cerr << endl;
}

void LanguageModel::EvaluateWhenApplied(const SCFG::Manager &mgr,
                                        const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                        FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

