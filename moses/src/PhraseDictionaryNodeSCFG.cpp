// $Id: PhraseDictionaryNodeSCFG.cpp 3566 2010-09-26 11:38:09Z pjwilliams $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "PhraseDictionaryNodeSCFG.h"
#include "TargetPhrase.h"
#include "PhraseDictionary.h"

namespace Moses
{
	
PhraseDictionaryNodeSCFG::~PhraseDictionaryNodeSCFG()
{
	delete m_targetPhraseCollection;
}

void PhraseDictionaryNodeSCFG::Prune(size_t tableLimit)
{
	// recusively prune
	for (TerminalMap::iterator p = m_sourceTermMap.begin(); p != m_sourceTermMap.end(); ++p)
	{
		p->second.Prune(tableLimit);
	}
	for (NonTerminalMap::iterator p = m_nonTermMap.begin(); p != m_nonTermMap.end(); ++p)
	{
		p->second.Prune(tableLimit);
	}
	
	// prune TargetPhraseCollection in this node
	if (m_targetPhraseCollection != NULL)
		m_targetPhraseCollection->Prune(true, tableLimit);
}

PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetOrCreateChild(const Word &sourceTerm)
{
	assert(!sourceTerm.IsNonTerminal());

	std::pair <TerminalMap::iterator,bool> insResult; 
	insResult = m_sourceTermMap.insert( std::make_pair(sourceTerm, PhraseDictionaryNodeSCFG()) );
	const TerminalMap::iterator &iter = insResult.first;
	PhraseDictionaryNodeSCFG &ret = iter->second;
	return &ret;
}
	
PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
{
	assert(sourceNonTerm.IsNonTerminal());
	assert(targetNonTerm.IsNonTerminal());

	NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
	std::pair <NonTerminalMap::iterator,bool> insResult; 
	insResult = m_nonTermMap.insert( std::make_pair(key, PhraseDictionaryNodeSCFG()) );
	const NonTerminalMap::iterator &iter = insResult.first;
	PhraseDictionaryNodeSCFG &ret = iter->second;
	return &ret;
}
	
const PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetChild(const Word &sourceTerm) const
{
	assert(!sourceTerm.IsNonTerminal());

	TerminalMap::const_iterator p = m_sourceTermMap.find(sourceTerm);
	return (p == m_sourceTermMap.end()) ? NULL : &p->second;
}

const PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const
{
	assert(sourceNonTerm.IsNonTerminal());
	assert(targetNonTerm.IsNonTerminal());

	NonTerminalMapKey key(sourceNonTerm, targetNonTerm);
	NonTerminalMap::const_iterator p = m_nonTermMap.find(key);
	return (p == m_nonTermMap.end()) ? NULL : &p->second;
}

std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNodeSCFG &node)
{
	out << node.GetTargetPhraseCollection();
	return out;
}

TO_STRING_BODY(PhraseDictionaryNodeSCFG)
	
}

