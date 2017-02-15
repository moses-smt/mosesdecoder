/*
 * LexicalReordering.cpp
 *
 *  Created on: 15 Dec 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "util/exception.hh"
#include "LexicalReordering.h"
#include "LRModel.h"
#include "PhraseBasedReorderingState.h"
#include "BidirectionalReorderingState.h"
#include "../../TranslationModel/PhraseTable.h"
#include "../../System.h"
#include "../../PhraseBased/PhraseImpl.h"
#include "../../PhraseBased/Manager.h"
#include "../../PhraseBased/Hypothesis.h"
#include "../../PhraseBased/TargetPhrases.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../legacy/InputFileStream.h"
#include "../../legacy/Util2.h"

#ifdef HAVE_CMPH
#include "../../TranslationModel/CompactPT/LexicalReorderingTableCompact.h"
#endif


using namespace std;

namespace Moses2
{

///////////////////////////////////////////////////////////////////////

LexicalReordering::LexicalReordering(size_t startInd, const std::string &line)
  : StatefulFeatureFunction(startInd, line)
  , m_blank(NULL)
  , m_propertyInd(-1)
  , m_coll(NULL)
  , m_configuration(NULL)
#ifdef HAVE_CMPH
  , m_compactModel(NULL)
#endif
{
  ReadParameters();
  assert(m_configuration);
  //assert(m_numScores == 6);
}

LexicalReordering::~LexicalReordering()
{
  delete m_coll;
  delete m_configuration;
#ifdef HAVE_CMPH
  delete m_compactModel;
#endif
}

void LexicalReordering::Load(System &system)
{
  MemPool &pool = system.GetSystemPool();

  if (m_propertyInd >= 0) {
    // Using integrate Lex RO. No loading needed
#ifdef HAVE_CMPH
  } else if (FileExists(m_path + ".minlexr")) {
    m_compactModel = new LexicalReorderingTableCompact(m_path + ".minlexr",
        m_FactorsF, m_FactorsE, m_FactorsC);
    m_blank = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, 0);
#endif
  } else {
    m_coll = new Coll();
    InputFileStream file(m_path);
    string line;
    size_t lineNum = 0;

    while (getline(file, line)) {
      if (++lineNum % 1000000 == 0) {
        cerr << lineNum << " ";
      }

      std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
      assert(toks.size() == 3);
      PhraseImpl *source = PhraseImpl::CreateFromString(pool, system.GetVocab(),
                           system, toks[0]);
      PhraseImpl *target = PhraseImpl::CreateFromString(pool, system.GetVocab(),
                           system, toks[1]);
      std::vector<SCORE> scores = Tokenize<SCORE>(toks[2]);
      std::transform(scores.begin(), scores.end(), scores.begin(),
                     TransformScore);
      std::transform(scores.begin(), scores.end(), scores.begin(), FloorScore);

      Key key(source, target);
      (*m_coll)[key] = scores;
    }
  }
}

void LexicalReordering::SetParameter(const std::string& key,
                                     const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else if (key == "type") {
    m_configuration = new LRModel(value, *this);
  } else if (key == "input-factor") {
    m_FactorsF = Tokenize<FactorType>(value);
  } else if (key == "output-factor") {
    m_FactorsE = Tokenize<FactorType>(value);
  } else if (key == "property-index") {
    m_propertyInd = Scan<int>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* LexicalReordering::BlankState(MemPool &pool, const System &sys) const
{
  FFState *ret = m_configuration->CreateLRState(pool);
  return ret;
}

void LexicalReordering::EmptyHypothesisState(FFState &state,
    const ManagerBase &mgr, const InputType &input,
    const Hypothesis &hypo) const
{
  BidirectionalReorderingState &stateCast =
    static_cast<BidirectionalReorderingState&>(state);
  stateCast.Init(NULL, hypo.GetTargetPhrase(), hypo.GetInputPath(), true,
                 &hypo.GetBitmap());
}

void LexicalReordering::EvaluateInIsolation(MemPool &pool, const System &system,
    const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void LexicalReordering::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
  UTIL_THROW2("Don't use with SCFG models");
}


void LexicalReordering::EvaluateAfterTablePruning(MemPool &pool,
    const TargetPhrases &tps, const Phrase<Moses2::Word> &sourcePhrase) const
{
  BOOST_FOREACH(const TargetPhraseImpl *tp, tps) {
    EvaluateAfterTablePruning(pool, *tp, sourcePhrase);
  }
}

void LexicalReordering::EvaluateAfterTablePruning(MemPool &pool,
    const TargetPhraseImpl &targetPhrase, const Phrase<Moses2::Word> &sourcePhrase) const
{
  if (m_propertyInd >= 0) {
    SCORE *scoreArr = targetPhrase.GetScoresProperty(m_propertyInd);
    targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
#ifdef HAVE_CMPH
  } else if (m_compactModel) {
    // using external compact binary model
    const Values values = m_compactModel->GetScore(sourcePhrase, targetPhrase,
                          *m_blank);
    if (values.size()) {
      assert(values.size() == m_numScores);

      SCORE *scoreArr = pool.Allocate<SCORE>(m_numScores);
      for (size_t i = 0; i < m_numScores; ++i) {
        scoreArr[i] = values[i];
      }
      targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
    } else {
      targetPhrase.ffData[m_PhraseTableInd] = NULL;
    }
#endif
  } else if (m_coll) {
    // using external memory model

    // cache data in target phrase
    const Values *values = GetValues(sourcePhrase, targetPhrase);
    assert(values->size() == m_numScores);

    if (values) {
      SCORE *scoreArr = pool.Allocate<SCORE>(m_numScores);
      for (size_t i = 0; i < m_numScores; ++i) {
        scoreArr[i] = (*values)[i];
      }
      targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
    } else {
      targetPhrase.ffData[m_PhraseTableInd] = NULL;
    }
  }
}

void LexicalReordering::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  const LRState &prevStateCast = static_cast<const LRState&>(prevState);
  prevStateCast.Expand(mgr, *this, hypo, m_PhraseTableInd, scores, state);
}

const LexicalReordering::Values *LexicalReordering::GetValues(
  const Phrase<Moses2::Word> &source, const Phrase<Moses2::Word> &target) const
{
  Key key(&source, &target);
  Coll::const_iterator iter;
  iter = m_coll->find(key);
  if (iter == m_coll->end()) {
    return NULL;
  } else {
    return &iter->second;
  }
}

void LexicalReordering::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

} /* namespace Moses2 */
