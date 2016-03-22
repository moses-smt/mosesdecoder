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

struct LexicalReorderingState : public FFState
{
  const InputPathBase *path;
  const TargetPhrase *targetPhrase;

  LexicalReorderingState()
  {
	  // uninitialised
  }


  size_t hash() const {
	// compare range address. All ranges are created in InputPathBase
    return (size_t) &path->range;
  }
  virtual bool operator==(const FFState& other) const {
	// compare range address. All ranges are created in InputPathBase
    const LexicalReorderingState &stateCast = static_cast<const LexicalReorderingState&>(other);
    return &path->range == &stateCast.path->range;
  }

  virtual std::string ToString() const
  {
	  return "";
  }

};

} /* namespace Moses2 */

