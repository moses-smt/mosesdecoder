#ifndef SEARCH_TYPES__
#define SEARCH_TYPES__

#include <cmath>

namespace search {

typedef float Score;
const Score kScoreInf = INFINITY;

// This could have been an enum but gcc wants 4 bytes.  
typedef bool ExtendDirection;
const ExtendDirection kExtendLeft = 0;
const ExtendDirection kExtendRight = 1;

} // namespace search

#endif // SEARCH_TYPES__
