/*
 * PhraseLR.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#pragma once
#include "../../legacy/FFState.h"
#include "../../InputPathBase.h"

namespace Moses2 {

class TargetPhrase;
class LexicalReordering;
class Hypothesis;

class LRState : public FFState
{
public:
  virtual void Expand(const System &system,
			  const LexicalReordering &ff,
			  const Hypothesis &hypo,
			  size_t phraseTableInd,
			  Scores &scores,
			  FFState &state) const = 0;

};

class PhraseBasedReorderingState : public LRState
{
public:
  const InputPathBase *path;
  const TargetPhrase *targetPhrase;

  PhraseBasedReorderingState()
  {
	  // uninitialised
  }

  size_t hash() const {
	// compare range address. All ranges are created in InputPathBase
    return (size_t) &path->range;
  }
  virtual bool operator==(const FFState& other) const {
	// compare range address. All ranges are created in InputPathBase
    const PhraseBasedReorderingState &stateCast = static_cast<const PhraseBasedReorderingState&>(other);
    return &path->range == &stateCast.path->range;
  }

  virtual std::string ToString() const
  {
	  return "";
  }

  void Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const;

protected:
  size_t  GetOrientation(Range const& cur) const;
  size_t  GetOrientation(Range const& prev, Range const& cur) const;

};

} /* namespace Moses2 */

