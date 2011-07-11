/*
 *  LatticeNode.h
 *  extract
 *
 *  Created by Hieu Hoang on 07/12/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <map>
#include "SyntaxTree.h"

class LatticeNode
{
	std::map<std::string, LatticeNode> m_coll;

public:
	void Add(const std::vector<std::string> &toks, const SyntaxTree &tree, size_t startPos, size_t numSymbols, size_t numNonTerms);

	bool Include(const std::vector<std::string> &toks, size_t pos) const;

};

