// $Id$
// vim:tabstop=2

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

#pragma once

#include <map>
#include <vector>
#include <iterator>
#include <utility>
#include <ostream>
#include "Word.h"
#include "TargetPhraseCollection.h"

namespace Moses
{

class PhraseDictionarySCFG;

/** One node of the PhraseDictionarySCFG structure
*/
class PhraseDictionaryNodeSCFG
{
	typedef std::map<Word, PhraseDictionaryNodeSCFG> TerminalMap;
	typedef std::pair<Word, Word> NonTerminalMapKey;
	typedef std::map<NonTerminalMapKey, PhraseDictionaryNodeSCFG> NonTerminalMap;

    friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);

	// only these classes are allowed to instantiate this class
	friend class PhraseDictionarySCFG;
	friend class std::map<Word, PhraseDictionaryNodeSCFG>;
	
protected:
	TerminalMap m_sourceTermMap;
	NonTerminalMap m_nonTermMap;
	mutable TargetPhraseCollection *m_targetPhraseCollection;
	
	PhraseDictionaryNodeSCFG()
		:m_targetPhraseCollection(NULL)
	{}
public:
	virtual ~PhraseDictionaryNodeSCFG();

	bool IsLeaf() const
	{ return m_sourceTermMap.empty() && m_nonTermMap.empty(); }

	void Sort(size_t tableLimit);
	PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceTerm);
	PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm);
	const PhraseDictionaryNodeSCFG *GetChild(const Word &sourceTerm) const;
	const PhraseDictionaryNodeSCFG *GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const;
	
	const TargetPhraseCollection *GetTargetPhraseCollection() const
	{	return m_targetPhraseCollection; }
	TargetPhraseCollection &GetOrCreateTargetPhraseCollection()
	{
		if (m_targetPhraseCollection == NULL)
			m_targetPhraseCollection = new TargetPhraseCollection();
		return *m_targetPhraseCollection;
	}

	// for mert
	void SetWeightTransModel(const PhraseDictionary *phraseDictionary
													, const std::vector<float> &weightT);

	TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeSCFG&);

}
