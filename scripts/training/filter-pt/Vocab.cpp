//
//  Vocab.cpp
//  filter-pt
//
//  Created by Hieu Hoang on 13/04/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "Vocab.h"

using namespace std;

const string *Vocab::Add(const string &factorString)
{
  // find string id
  const string *ptrString=&(*m_factorStringCollection.insert(factorString).first);
  return ptrString;
}

