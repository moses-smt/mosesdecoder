// $Id: PhraseDictionaryNodeSCFG.h 3797 2011-01-13 00:25:10Z pjwilliams $
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

#ifdef HAVE_BOOST
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>
#endif

namespace Moses
{

class PhraseDictionarySCFG;

#ifdef HAVE_BOOST
class TerminalHasher
{
public:
    // Generate a hash value for a word representing a terminal.  It's
    // assumed that the same subset of factors will be active for all words
    // that are hashed.
    size_t operator()(const Word & t) const
    {
        size_t seed = 0;
        for (size_t i = 0; i < MAX_NUM_FACTORS; ++i)
        {
            const Factor * f = t[i];
            if (f)
            {
                boost::hash_combine(seed, *f);
            }
        }
        return seed;
    }
};

class TerminalEqualityPred
{
public:
    // Equality predicate for comparing words representing terminals.  As
    // with the hasher, it's assumed that all words will have the same
    // subset of active factors.
    bool operator()(const Word & t1, const Word & t2) const
    {
        for (size_t i = 0; i < MAX_NUM_FACTORS; ++i)
        {
            const Factor * f1 = t1[i];
            const Factor * f2 = t2[i];
            if (f1 && f1->Compare(*f2))
            {
                return false;
            }
        }
        return true;
    }
};

class NonTerminalMapKeyHasher
{
public:
    size_t operator()(const std::pair<Word, Word> & k) const
    {
        // Assumes that only the first factor of each Word is relevant.
        const Word & w1 = k.first;
        const Word & w2 = k.second;
        const Factor * f1 = w1[0];
        const Factor * f2 = w2[0];
        size_t seed = 0;
        boost::hash_combine(seed, *f1);
        boost::hash_combine(seed, *f2);
        return seed;
    }
};

class NonTerminalMapKeyEqualityPred
{
public:
    bool operator()(const std::pair<Word, Word> & k1,
                    const std::pair<Word, Word> & k2) const
    {
        // Compare first non-terminal of each key.  Assumes that for Words
        // representing non-terminals only the first factor is relevant.
        {
            const Word & w1 = k1.first;
            const Word & w2 = k2.first;
            const Factor * f1 = w1[0];
            const Factor * f2 = w2[0];
            if (f1->Compare(*f2))
            {
                return false;
            }
        }
        // Compare second non-terminal of each key.
        {
            const Word & w1 = k1.second;
            const Word & w2 = k2.second;
            const Factor * f1 = w1[0];
            const Factor * f2 = w2[0];
            if (f1->Compare(*f2))
            {
                return false;
            }
        }
        return true;
    }
};
#endif

/** One node of the PhraseDictionarySCFG structure
*/
class PhraseDictionaryNodeSCFG
{
public:
    typedef std::pair<Word, Word> NonTerminalMapKey;

#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
    typedef boost::unordered_map<Word,
                                 PhraseDictionaryNodeSCFG,
                                 TerminalHasher,
                                 TerminalEqualityPred> TerminalMap;

    typedef boost::unordered_map<NonTerminalMapKey,
                                 PhraseDictionaryNodeSCFG,
                                 NonTerminalMapKeyHasher,
                                 NonTerminalMapKeyEqualityPred> NonTerminalMap;
#else
    typedef std::map<Word, PhraseDictionaryNodeSCFG> TerminalMap;
    typedef std::map<NonTerminalMapKey, PhraseDictionaryNodeSCFG> NonTerminalMap;
#endif

private:
    friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);

	// only these classes are allowed to instantiate this class
	friend class PhraseDictionarySCFG;
	friend class std::map<Word, PhraseDictionaryNodeSCFG>;
	
protected:
	TerminalMap m_sourceTermMap;
	NonTerminalMap m_nonTermMap;
	TargetPhraseCollection *m_targetPhraseCollection;
	
	PhraseDictionaryNodeSCFG()
		:m_targetPhraseCollection(NULL)
	{}
public:
	virtual ~PhraseDictionaryNodeSCFG();

	bool IsLeaf() const
	{ return m_sourceTermMap.empty() && m_nonTermMap.empty(); }

	void Prune(size_t tableLimit);
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

    const NonTerminalMap & GetNonTerminalMap() const
    { return m_nonTermMap; }

	TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeSCFG&);

}
