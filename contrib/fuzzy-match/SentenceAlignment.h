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
#include "Vocabulary.h"

extern Vocabulary vocabulary;

struct SentenceAlignment
{
  int count;
  vector< WORD_ID > target;
  vector< pair<int,int> > alignment;
  
  SentenceAlignment()
  {}
  
  string getTargetString() const
  {
    stringstream strme;
    for (size_t i = 0; i < target.size(); ++i) {
      const WORD &word = vocabulary.GetWord(target[i]);
      strme << word << " ";
    }
    return strme.str();
  }
  
  string getAlignmentString() const
  {
    stringstream strme;
    for (size_t i = 0; i < alignment.size(); ++i) {
      const pair<int,int> &alignPair = alignment[i];
      strme << alignPair.first << "-" << alignPair.second << " ";
    }
    return strme.str();
  }
  
};

#endif
