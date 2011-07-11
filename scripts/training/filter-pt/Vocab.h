//
//  Vocab.h
//  filter-pt
//
//  Created by Hieu Hoang on 13/04/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#pragma once

#include <set>
#include <string>

class Vocab
{
  typedef std::set<std::string> StringSet;
  
  StringSet m_factorStringCollection; /**< collection of unique string used by factors */
  
public:
  
  const std::string *Add(const std::string &factorString);
  
};
