#include "UtilESM.h"

#include <boost/foreach.hpp>

namespace Moses
{

bool overlap(const MinPhrase& p1, const MinPhrase& p2) {
  return ((p1[0] <= p2[0] && p2[0] <= p1[0] + p1[2]) || (p2[0] <= p1[0] && p1[0] <= p2[0] + p2[2]) ||
    (p1[1] <= p2[1] && p2[1] <= p1[1] + p1[3]) || (p2[1] <= p1[1] && p1[1] <= p2[1] + p2[3]));
}

MinPhrase combine(const MinPhrase& p1, const MinPhrase& p2) {
  MinPhrase c(4, 0);
  c[0] = std::min(p1[0], p2[0]);
  c[1] = std::min(p1[1], p2[1]);
  c[2] = std::max(p1[0] + p1[2] - c[0], p2[0] + p2[2] - c[0]);
  c[3] = std::max(p1[1] + p1[3] - c[1], p2[1] + p2[3] - c[1]);
  return c;
}

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>& source,
                      const std::vector<std::string>& target,
                      const std::vector<size_t>& alignment) {
  
  MinPhrases minPhrases;
  for(size_t i = 0; i < alignment.size(); i += 2) {
    MinPhrase phrase(4, 0);
    phrase[0] = alignment[i];
    phrase[1] = alignment[i + 1];
    minPhrases.insert(phrase);
  }
  
  // @TODO: optimize this speed-wise
  repeat:
  BOOST_FOREACH(MinPhrase p1, minPhrases) {
    BOOST_FOREACH(MinPhrase p2, minPhrases) {
      if(p1 != p2) {
        if(overlap(p1, p2)) {
          minPhrases.erase(p1);
          minPhrases.erase(p2);   
          minPhrases.insert(combine(p1, p2));
          goto repeat;
        }
      }
    }
  }
  
  // @TODO: Handle unaligned words
  
  std::vector<std::string> edits;
  return edits;
}

}