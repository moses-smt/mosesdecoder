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
  const Range *range;

  LexicalReorderingState()
  {
	  // uninitialised
  }


  size_t hash() const {
    return (size_t) range;
  }
  virtual bool operator==(const FFState& other) const {
	  const LexicalReorderingState &stateCast = static_cast<const LexicalReorderingState&>(other);
	  return range == stateCast.range;
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
	stateCast.range = &hypo.GetInputPath().range;
}

void LexicalReordering::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const FFState &prevState,
  Scores &scores,
  FFState &state) const
{
  const Phrase &source = hypo.GetInputPath().subPhrase;
  const Phrase &target = hypo.GetTargetPhrase();

  const LexicalReordering::Values *values = GetValues(source, target);

  if (values) {
	  size_t orientation;
	  vector<SCORE> scoreVec(6, 0);

	  const Range &currRange = hypo.GetInputPath().range;

	  const Hypothesis *prevHypo = hypo.GetPrevHypo();
	  assert(prevHypo);
	  const Range &prevRange = prevHypo->GetInputPath().range;

	  if (prevRange.GetStartPos() != NOT_FOUND) {
		  const Range &prevRange = prevHypo->GetInputPath().range;
		  orientation = GetOrientation(prevRange, currRange);
		  scoreVec[orientation] = (*values)[orientation];
		  scoreVec[2 + orientation] = (*values)[2 + orientation];
	  }
	  else {
		  orientation = GetOrientation(currRange);
		  scoreVec[orientation] = (*values)[orientation];
	  }

	  scores.PlusEquals(mgr.system, *this, scoreVec);
  }
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
