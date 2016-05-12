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
#include "../../legacy/Range.h"
#include "../../Vector.h"

namespace Moses2
{
class MemPool;

class ReorderingStack
{
private:

  Vector<Range> m_stack;

public:
  ReorderingStack(MemPool &pool);

  size_t hash() const;
  bool operator==(const ReorderingStack& other) const;

  void Init();
  int ShiftReduce(const Range &input_span);

private:
  void Reduce(Range input_span);
};

}
