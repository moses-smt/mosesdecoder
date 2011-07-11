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

using namespace std;

void Lattice::Add(const std::vector<std::string> &toks, const SyntaxTree &tree, Vocab &vocab)
{
	size_t len = toks.size();
	for (size_t startPos = 0; startPos < len; ++startPos)
	{
		m_root.Add(toks, tree, startPos, 0, 0, vocab);
	}
	
}

bool Lattice::Include(const std::string &line, Vocab &vocab)
{
	vector<string> toks = tokenize(line.c_str());
	bool ret = m_root.Include(toks, 0, vocab);
	return ret;
}

