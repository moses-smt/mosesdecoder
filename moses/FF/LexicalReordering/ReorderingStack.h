/*
 * ReorderingStack.h
 ** Author: Ankit K. Srivastava
 ** Date: Jan 26, 2010
 */

#pragma once

//#include <string>
#include <vector>
//#include "Factor.h"
//#include "Phrase.h"
//#include "TypeDef.h"
//#include "Util.h"
#include "moses/Range.h"

namespace Moses
{

/** @todo what is this?
 */
class ReorderingStack
{
private:

  std::vector<Range> m_stack;

public:

  size_t hash() const;
  bool operator==(const ReorderingStack& other) const;

  int ShiftReduce(Range input_span);

private:
  void Reduce(Range input_span);
};


}
