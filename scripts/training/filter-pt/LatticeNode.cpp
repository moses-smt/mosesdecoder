/*
 *  LatticeNode.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 07/12/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "LatticeNode.h"
#include "filter-pt.h"

using namespace std;

void LatticeNode::Add(const std::vector<std::string> &toks
                      , const SyntaxTree &tree
                      , size_t startPos
                      , size_t numSymbols
                      , size_t numNonTerms
                      , Vocab &vocab)
{
	if (numSymbols > MAX_SOURCE_SYMBOLS)
		return;
	if (numNonTerms > MAX_NT)
		return;
	
	size_t len = toks.size();
	for (size_t endPos = startPos; endPos < len; ++endPos)
	{
		if (endPos == startPos)
		{ // add word too
			const string &word = toks[startPos];
      const string *wordVocab = vocab.Add(word);
      
			LatticeNode &node = m_coll[wordVocab];
			node.Add(toks, tree, endPos + 1, numSymbols + 1, numNonTerms, vocab);
		}
		
		// non terms
		const std::vector< SyntaxNode* > &nodes = tree.GetNodes(startPos, endPos);
		std::vector< SyntaxNode* >::const_iterator iterNodes;
		for (iterNodes = nodes.begin(); iterNodes != nodes.end(); ++iterNodes)
		{
			SyntaxNode &treeNode = **iterNodes;
			string label = treeNode.GetLabel();
      const string *wordVocab = vocab.Add(label);

			LatticeNode &node = m_coll[wordVocab];
			node.Add(toks, tree, endPos + 1, numSymbols + 1, numNonTerms + 1, vocab);
		}
	}
	
}

bool LatticeNode::Include(const std::vector<std::string> &toks, size_t pos, Vocab &vocab) const
{
	map<const std::string*, LatticeNode>::const_iterator iter;
	
	const string &symbol = toks[pos];
	if (symbol.substr(0, 1) == "[" && symbol.substr(symbol.size() - 1, 1) == "]")
	{
		size_t closingPos = symbol.find_first_of("]", 1);

		if (closingPos == symbol.size() - 1)
		{ // lhs
			return true;
		}
		else
		{
			string sourceNT = symbol.substr(1, closingPos - 1);
      const string *wordVocab = vocab.Add(sourceNT);
      
			iter = m_coll.find(wordVocab);
		}

	}
	else
	{ // word
    const string *wordVocab = vocab.Add(symbol);
		iter = m_coll.find(wordVocab);
	}

	if (iter == m_coll.end())
	{
		return false;
	}
	else 
	{
		const LatticeNode &node = iter->second;
		bool ret = node.Include(toks, pos + 1, vocab);
		return ret;
	}

	
	
}


