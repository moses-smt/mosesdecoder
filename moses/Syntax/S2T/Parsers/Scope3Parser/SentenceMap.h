#pragma once

#include <vector>

#include <boost/unordered_map.hpp>

#include "moses/Syntax/SymbolEqualityPred.h"
#include "moses/Syntax/SymbolHasher.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{

struct PVertex;

namespace S2T
{

// FIXME Check SymbolHasher does the right thing here
typedef boost::unordered_map<Word, std::vector<const PVertex *>, SymbolHasher,
        SymbolEqualityPred> SentenceMap;

} // namespace S2T
} // namespace Syntax
} // namespace Moses
