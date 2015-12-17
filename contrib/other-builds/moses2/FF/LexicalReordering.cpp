/*
 * LexicalReordering.cpp
 *
 *  Created on: 15 Dec 2015
 *      Author: hieu
 */

#include "LexicalReordering.h"
#include "../System.h"
#include "../Search/Manager.h"
#include "../legacy/InputFileStream.h"

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
{
	ReadParameters();
	assert(m_numScores == 6);
}

LexicalReordering::~LexicalReordering()
{
	// TODO Auto-generated destructor stub
}

void LexicalReordering::Load(System &system)
{
  InputFileStream file(m_path);
  string line;
  size_t lineNum = 0;

  while(getline(file, line)) {
	if (++lineNum % 1000000 == 0) {
		cerr << lineNum << " ";
	}

	std::vector<std::string> toks = TokenizeMultiCharSeparator(line, "|||");
	assert(toks.size() == 3);
	PhraseImpl *source = PhraseImpl::CreateFromString(system.systemPool, system.GetVocab(), system, toks[0]);
	PhraseImpl *target = PhraseImpl::CreateFromString(system.systemPool, system.GetVocab(), system, toks[1]);
	std::vector<SCORE> scores = Tokenize<SCORE>(toks[2]);
    std::transform(scores.begin(), scores.end(), scores.begin(), TransformScore);

	Key key(source, target);
	m_coll[key] = scores;
  }
}

void LexicalReordering::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "type") {

  }
  else if (key == "input-factor") {

  }
  else if (key == "output-factor") {

  }

  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* LexicalReordering::BlankState(const Manager &mgr, const InputType &input) const
{
  MemPool &pool = mgr.GetPool();
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
		Scores *estimatedScores) const
{
  // cache data in target phrase
  const Values *values = GetValues(source, targetPhrase);
  if (values) {
    //scoreVec[orientation] = (*values)[orientation];
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

  vector<SCORE> scoreVec(m_numScores, 0);

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
  const Phrase &source = hypo.GetInputPath().subPhrase;
  const Phrase &target = hypo.GetTargetPhrase();

  const Values *values = GetValues(source, target);
  if (values) {
	  scoreVec[orientation] = (*values)[orientation];
  }

  // forwards
  if (prevRange->GetStartPos() != NOT_FOUND) {
	  const Phrase &source = prevStateCast.path->subPhrase;
	  const Phrase &target = *prevStateCast.targetPhrase;

	  const Values *values = GetValues(source, target);
	  if (values) {
		  scoreVec[orientation + 3] = (*values)[orientation + 3];
	  }
  }

  scores.PlusEquals(mgr.system, *this, scoreVec);

}

const LexicalReordering::Values *LexicalReordering::GetValues(const Phrase &source, const Phrase &target) const
{
	Key key(&source, &target);
	Coll::const_iterator iter;
	iter = m_coll.find(key);
	if (iter == m_coll.end()) {
		return NULL;
	}
	else {
		return &iter->second;
	}
}

size_t LexicalReordering::GetOrientation(Range const& cur) const
{
  return (cur.GetStartPos() == 0) ? 0 : 1;
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
