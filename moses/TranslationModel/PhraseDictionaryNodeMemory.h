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
#include "moses/Word.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Terminal.h"
#include "moses/NonTerminal.h"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

namespace Moses
{

class PhraseDictionaryMemory;
class PhraseDictionaryScope3;
class PhraseDictionaryFuzzyMatch;

//! @todo why?
class NonTerminalMapKeyHasher
{
public:
  size_t operator()(const std::pair<Word, Word> & k) const {
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

//! @todo why?
class NonTerminalMapKeyEqualityPred
{
public:
  bool operator()(const std::pair<Word, Word> & k1,
                  const std::pair<Word, Word> & k2) const {
    // Compare first non-terminal of each key.  Assumes that for Words
    // representing non-terminals only the first factor is relevant.
    {
      const Word & w1 = k1.first;
      const Word & w2 = k2.first;
      const Factor * f1 = w1[0];
      const Factor * f2 = w2[0];
      if (f1->Compare(*f2)) {
        return false;
      }
    }
    // Compare second non-terminal of each key.
    {
      const Word & w1 = k1.second;
      const Word & w2 = k2.second;
      const Factor * f1 = w1[0];
      const Factor * f2 = w2[0];
      if (f1->Compare(*f2)) {
        return false;
      }
    }
    return true;
  }
};

/** One node of the PhraseDictionaryMemory structure
*/
class PhraseDictionaryNodeMemory
{
public:
  typedef std::pair<Word, Word> NonTerminalMapKey;

#if defined(BOOST_VERSION) && (BOOST_VERSION >= 104200)
  typedef boost::unordered_map<Word,
          PhraseDictionaryNodeMemory,
          TerminalHasher,
          TerminalEqualityPred> TerminalMap;

#if defined(UNLABELLED_SOURCE)
  typedef boost::unordered_map<Word,
          PhraseDictionaryNodeMemory,
          NonTerminalHasher,
          NonTerminalEqualityPred> NonTerminalMap;
#else
  typedef boost::unordered_map<NonTerminalMapKey,
          PhraseDictionaryNodeMemory,
          NonTerminalMapKeyHasher,
          NonTerminalMapKeyEqualityPred> NonTerminalMap;
#endif
#else
  typedef std::map<Word, PhraseDictionaryNodeMemory> TerminalMap;
#if defined(UNLABELLED_SOURCE)
  typedef std::map<Word, PhraseDictionaryNodeMemory> NonTerminalMap;
#else
  typedef std::map<NonTerminalMapKey, PhraseDictionaryNodeMemory> NonTerminalMap;
#endif
#endif

private:
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMemory&);
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryScope3&);
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryFuzzyMatch&);

  TerminalMap m_sourceTermMap;
  NonTerminalMap m_nonTermMap;
  TargetPhraseCollection::shared_ptr m_targetPhraseCollection;


public:
  PhraseDictionaryNodeMemory()
    : m_targetPhraseCollection(new TargetPhraseCollection) { }

  bool IsLeaf() const {
    return m_sourceTermMap.empty() && m_nonTermMap.empty();
  }

  void Prune(size_t tableLimit);
  void Sort(size_t tableLimit);
  PhraseDictionaryNodeMemory *GetOrCreateChild(const Word &sourceTerm);
  const PhraseDictionaryNodeMemory *GetChild(const Word &sourceTerm) const;
#if defined(UNLABELLED_SOURCE)
  PhraseDictionaryNodeMemory *GetOrCreateNonTerminalChild(const Word &targetNonTerm);
  const PhraseDictionaryNodeMemory *GetNonTerminalChild(const Word &targetNonTerm) const;
#else
  PhraseDictionaryNodeMemory *GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm);
  const PhraseDictionaryNodeMemory *GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const;
#endif

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection() const {
    return m_targetPhraseCollection;
  }
  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection() {
    return m_targetPhraseCollection;
  }

  const TerminalMap & GetTerminalMap() const {
    return m_sourceTermMap;
  }

  const NonTerminalMap & GetNonTerminalMap() const {
    return m_nonTermMap;
  }

  void Remove();

  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeMemory&);

}
