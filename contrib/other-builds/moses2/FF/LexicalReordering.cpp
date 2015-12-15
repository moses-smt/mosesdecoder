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
  LexicalReorderingState()
  {
	  // uninitialised
  }


  size_t hash() const {
    return 0;
  }
  virtual bool operator==(const FFState& other) const {
	  return true;
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

void LexicalReordering::EmptyHypothesisState(FFState &state, const Manager &mgr, const InputType &input) const
{

}

void LexicalReordering::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const FFState &prevState,
  Scores &scores,
	FFState &state) const
{

}


} /* namespace Moses2 */
