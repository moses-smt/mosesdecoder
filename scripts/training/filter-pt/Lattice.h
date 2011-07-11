/*
 *  Lattice.h
 *  extract
 *
 *  Created by Hieu Hoang on 07/12/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "LatticeNode.h"
#include <vector>
#include <string>
#include "Vocab.h"

class Lattice
{
	LatticeNode m_root;
public:
	void Add(const std::vector<std::string> &toks, const SyntaxTree &tree, Vocab &vocab);
	bool Include(const std::string &line, Vocab &vocab);
};

