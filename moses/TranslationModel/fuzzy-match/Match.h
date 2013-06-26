//
//  Match.h
//  fuzzy-match
//
//  Created by Hieu Hoang on 25/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef fuzzy_match_Match_h
#define fuzzy_match_Match_h

namespace tmmt
{

/* data structure for n-gram match between input and corpus */

class Match
{
public:
  int input_start;
  int input_end;
  int tm_start;
  int tm_end;
  int min_cost;
  int max_cost;
  int internal_cost;
  Match( int is, int ie, int ts, int te, int min, int max, int i )
    :input_start(is), input_end(ie), tm_start(ts), tm_end(te), min_cost(min), max_cost(max), internal_cost(i) {
  }
};

}

#endif
