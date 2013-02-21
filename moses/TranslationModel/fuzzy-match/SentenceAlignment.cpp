//
//  SentenceAlignment.cpp
//  moses
//
//  Created by Hieu Hoang on 26/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "SentenceAlignment.h"

namespace tmmt
{
std::string SentenceAlignment::getTargetString(const Vocabulary &vocab) const
{
  std::stringstream strme;
  for (size_t i = 0; i < target.size(); ++i) {
    const WORD &word = vocab.GetWord(target[i]);
    strme << word << " ";
  }
  return strme.str();
}

}
