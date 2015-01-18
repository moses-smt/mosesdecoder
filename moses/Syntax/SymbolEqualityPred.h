#pragma once

#include "moses/Factor.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{

// Assumes that only the first factor is relevant.  i.e. factored decoding will
// *not* work in moses_chart unless this is changed (among other things).
class SymbolEqualityPred
{
public:
  bool operator()(const Word &s1, const Word &s2) const {
    const Factor *f1 = s1[0];
    const Factor *f2 = s2[0];
    return !(f1->Compare(*f2));
  }
};

}  // namespace Syntax
}  // namespace Moses
