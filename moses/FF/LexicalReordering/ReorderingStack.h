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
#include "moses/WordsRange.h"

namespace Moses
{

/** @todo what is this?
 */
class ReorderingStack
{
private:

  std::vector<WordsRange> m_stack;

public:

  size_t hash() const;
  bool operator==(const ReorderingStack& other) const;

  int ShiftReduce(WordsRange input_span);

private:
  void Reduce(WordsRange input_span);
};


}
