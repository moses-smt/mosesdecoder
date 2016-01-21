/*
 * LexicalReordering.cpp
 *
 *  Created on: 15 Dec 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "LexicalReordering.h"
#include "../TranslationModel/PhraseTable.h"
#include "../System.h"
#include "../Search/Manager.h"
#include "../legacy/InputFileStream.h"
#include "../legacy/Util2.h"
#include "../legacy/CompactPT/LexicalReorderingTableCompact.h"

using namespace std;

namespace Moses2 {

struct LexicalReorderingState : public FFState
{
  const InputPath *path;
  const TargetPhrase *targetPhrase;

  LexicalReorderingState()
  {
	  // uninitialised
  }


  size_t hash() const {
	// compare range address. All ranges are created in InputPath
    return (size_t) &path->range;
  }
  virtual bool operator==(const FFState& other) const {
	// compare range address. All ranges are created in InputPath
    const LexicalReorderingState &stateCast = static_cast<const LexicalReorderingState&>(other);
    return &path->range == &stateCast.path->range;
  }

  virtual std::string ToString() const
  {
	  return "";
  }

};


///////////////////////////////////////////////////////////////////////

LexicalReordering::LexicalReordering(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
,m_compactModel(NULL)
,m_coll(NULL)
,m_propertyInd(-1)
{
	ReadParameters();
	assert(m_numScores == 6);
}

LexicalReordering::~LexicalReordering()
{
	delete m_compactModel;
	delete m_coll;
}

void LexicalReordering::Load(System &system)
{
  MemPool &pool = system.GetSystemPool();

  if (m_propertyInd >= 0) {
	  // Using integrate Lex RO. No loading needed
  }
  else if (FileExists(m_path + ".minlexr") ) {
	  m_compactModel = new LexicalReorderingTableCompact(m_path + ".minlexr", m_FactorsF,
			  m_FactorsE, m_FactorsC);
	  m_blank = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, 0);
  }
  else {
	  m_coll = new Coll();
	  InputFileStream file(m_path);
	  string line;
	  size_t lineNum = 0;

	  while(getline(file, line)) {
		if (++lineNum % 1000000 == 0) {
			cerr << lineNum << " ";
		}

		std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
		assert(toks.size() == 3);
		PhraseImpl *source = PhraseImpl::CreateFromString(pool, system.GetVocab(), system, toks[0]);
		PhraseImpl *target = PhraseImpl::CreateFromString(pool, system.GetVocab(), system, toks[1]);
		std::vector<SCORE> scores = Tokenize<SCORE>(toks[2]);
		std::transform(scores.begin(), scores.end(), scores.begin(), TransformScore);
		std::transform(scores.begin(), scores.end(), scores.begin(), FloorScore);

		Key key(source, target);
		(*m_coll)[key] = scores;
	  }
  }
}

void LexicalReordering::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "type") {
	  // ignore. only do 1 type
  }
  else if (key == "input-factor") {
	  m_FactorsF = Tokenize<FactorType>(value);
  }
  else if (key == "output-factor") {
	  m_FactorsE = Tokenize<FactorType>(value);
  }
  else if (key == "property-index") {
	  m_propertyInd = Scan<int>(value);
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* LexicalReordering::BlankState(MemPool &pool) const
{
  return new (pool.Allocate<LexicalReorderingState>()) LexicalReorderingState();
}

void LexicalReordering::EmptyHypothesisState(FFState &state,
		const Manager &mgr,
		const InputType &input,
		const Hypothesis &hypo) const
{
	LexicalReorderingState &stateCast = static_cast<LexicalReorderingState&>(state);
	stateCast.path = &hypo.GetInputPath();
	stateCast.targetPhrase = &hypo.GetTargetPhrase();
}

void LexicalReordering::EvaluateInIsolation(MemPool &pool,
		const System &system,
		const Phrase &source,
		const TargetPhrase &targetPhrase,
		Scores &scores,
		SCORE *estimatedScore) const
{
}

void LexicalReordering::EvaluateAfterTablePruning(MemPool &pool,
		const TargetPhrases &tps,
		const Phrase &sourcePhrase) const
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  EvaluateAfterTablePruning(pool, *tp, sourcePhrase);
  }
}

void LexicalReordering::EvaluateAfterTablePruning(MemPool &pool,
		const TargetPhrase &targetPhrase,
		const Phrase &sourcePhrase) const
{
  if (m_propertyInd >= 0) {
	  SCORE *scoreArr = targetPhrase.GetScoresProperty(m_propertyInd);
	  targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
  }
  else if (m_compactModel) {
	  // using external compact binary model
	  const Values values = m_compactModel->GetScore(sourcePhrase, targetPhrase, *m_blank);
	  if (values.size()) {
		assert(values.size() == 6);
		SCORE *scoreArr = pool.Allocate<SCORE>(6);
		for (size_t i = 0; i < 6; ++i) {
			scoreArr[i] = values[i];
		}
		targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
	  }
	  else {
		targetPhrase.ffData[m_PhraseTableInd] = NULL;
	  }
  }
  else if (m_coll) {
	  // using external memory model

	  // cache data in target phrase
	  const Values *values = GetValues(sourcePhrase, targetPhrase);
	  if (values) {
		SCORE *scoreArr = pool.Allocate<SCORE>(6);
		for (size_t i = 0; i < 6; ++i) {
			scoreArr[i] = (*values)[i];
		}
		targetPhrase.ffData[m_PhraseTableInd] = scoreArr;
	  }
	  else {
		targetPhrase.ffData[m_PhraseTableInd] = NULL;
	  }
  }
}

void LexicalReordering::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const FFState &prevState,
  Scores &scores,
  FFState &state) const
{
  const LexicalReorderingState &prevStateCast = static_cast<const LexicalReorderingState&>(prevState);
  LexicalReorderingState &stateCast = static_cast<LexicalReorderingState&>(state);

  const Range &currRange = hypo.GetInputPath().range;
  stateCast.path = &hypo.GetInputPath();
  stateCast.targetPhrase = &hypo.GetTargetPhrase();

  // calc orientation
  size_t orientation;
  const Range *prevRange = &prevStateCast.path->range;
  assert(prevRange);
  if (prevRange->GetStartPos() == NOT_FOUND) {
	  orientation = GetOrientation(currRange);
  }
  else {
	  orientation = GetOrientation(*prevRange, currRange);
  }

  // backwards
  const TargetPhrase &target = hypo.GetTargetPhrase();

  const SCORE *values = (const SCORE *) target.ffData[m_PhraseTableInd];
  if (values) {
	  scores.PlusEquals(mgr.system, *this, values[orientation], orientation);
  }

  // forwards
  if (prevRange->GetStartPos() != NOT_FOUND) {
	  const TargetPhrase &prevTarget = *prevStateCast.targetPhrase;
	  const SCORE *prevValues = (const SCORE *) prevTarget.ffData[m_PhraseTableInd];

	  if (prevValues) {
		  scores.PlusEquals(mgr.system, *this, prevValues[orientation + 3], orientation + 3);
	  }
  }

}

const LexicalReordering::Values *LexicalReordering::GetValues(const Phrase &source, const Phrase &target) const
{
	Key key(&source, &target);
	Coll::const_iterator iter;
	iter = m_coll->find(key);
	if (iter == m_coll->end()) {
		return NULL;
	}
	else {
		return &iter->second;
	}
}

size_t LexicalReordering::GetOrientation(Range const& cur) const
{
  return (cur.GetStartPos() == 0) ? 0 : 2;
}

size_t LexicalReordering::GetOrientation(Range const& prev, Range const& cur) const
{
  if (cur.GetStartPos() == prev.GetEndPos() + 1) {
	  // monotone
	  return 0;
  }
  else if (prev.GetStartPos() ==  cur.GetEndPos() + 1) {
	  // swap
	  return 1;
  }
  else {
	  // discontinuous
	  return 2;
  }
}

} /* namespace Moses2 */
