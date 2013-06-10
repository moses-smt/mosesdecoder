//
//  SentenceAlignment.h
//  fuzzy-match
//
//  Created by Hieu Hoang on 25/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef fuzzy_match_SentenceAlignment_h
#define fuzzy_match_SentenceAlignment_h

#include <sstream>
#include <vector>
#include "Vocabulary.h"

namespace tmmt
{

struct SentenceAlignment {
  int count;
  std::vector< WORD_ID > target;
  std::vector< std::pair<int,int> > alignment;

  SentenceAlignment() {
  }

  std::string getTargetString(const Vocabulary &vocab) const;

  std::string getAlignmentString() const {
    std::stringstream strme;
    for (size_t i = 0; i < alignment.size(); ++i) {
      const std::pair<int,int> &alignPair = alignment[i];
      strme << alignPair.first << "-" << alignPair.second << " ";
    }
    return strme.str();
  }

};

}

#endif
