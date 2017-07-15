#pragma once

#include <boost/functional/hash.hpp>

#include "moses/Factor.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{

// Assumes that only the first factor is relevant.  i.e. factored decoding will
// *not* work in moses_chart unless this is changed (among other things).
class SymbolHasher
{
public:
  std::size_t operator()(const Word &s) const {
    const Factor *f = s[0];
    return hash_value(*f);
  }
};

}  // namespace Syntax
}  // namespace Moses
