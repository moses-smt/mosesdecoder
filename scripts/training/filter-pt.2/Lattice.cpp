/*
 *  Lattice.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 07/12/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Lattice.h"
#include "SyntaxTree.h"
#include "tables-core.h"
#include "filter-pt.h"
#include "murmur_hash.hh"

using namespace std;

void Lattice::Add(const std::vector<std::string> &toks, const SyntaxTree &tree)
{
	size_t len = toks.size();
	for (size_t startPos = 0; startPos < len; ++startPos)
	{
    string prefix;
    size_t stopPos = min(len, startPos + MAX_SOURCE_SYMBOLS);
    for (size_t endPos = startPos; endPos < stopPos; ++endPos)
    {
      prefix += toks[endPos] + " ";
      size_t len = prefix.size();
      
      uint64_t key = util::MurmurHashNative(prefix.c_str(), len, HASH_KEY);
      m_keys.insert(key);
      
    }
	}
	
}

bool Lattice::Include(const std::string &line)
{
	vector<string> toks = tokenize(line.c_str());
	//bool ret = m_root.Include(toks, 0);
	//return ret;

}

