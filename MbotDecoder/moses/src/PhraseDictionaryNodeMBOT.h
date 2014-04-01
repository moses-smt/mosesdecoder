// $Id: PhraseDictionaryNodeMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>
#include <boost/tuple/tuple.hpp>
#include "PhraseDictionaryNodeSCFG.h"
#include "MBOTWord.h"

namespace Moses
{

class PhraseDictionarySCFG;
class PhraseDictionaryMBOT;

/** One node of the PhraseDictionaryMBOT structure
*/
class PhraseDictionaryNodeMBOT : public PhraseDictionaryNodeSCFG
{
public:

typedef std::pair<Word, std::vector<Word> > NonTerminalMapKeyMBOT;
typedef std::map<Word, PhraseDictionaryNodeMBOT> TerminalMapMBOT;
typedef std::map<NonTerminalMapKeyMBOT, PhraseDictionaryNodeMBOT> NonTerminalMapMBOT;

private:
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionarySCFG&);
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryMBOT&);
  friend std::ostream& operator<<(std::ostream&, const TargetPhraseCollection&);

  // only these classes are allowed to instantiate this class
  //friend class PhraseDictionarySCFG;
  friend class PhraseDictionaryMBOT;
  friend class std::map<Word, PhraseDictionaryNodeMBOT>;

protected:
  NonTerminalMapMBOT m_mbotNonTermMap;
  TerminalMapMBOT m_mbotSourceTermMap;
  TargetPhraseCollection *m_mbotTargetPhraseCollection;

  PhraseDictionaryNodeMBOT()
    : PhraseDictionaryNodeSCFG()
    , m_mbotTargetPhraseCollection(NULL)
  {}

public:
  ~PhraseDictionaryNodeMBOT();

  bool IsLeafMBOT() const {
    return m_mbotSourceTermMap.empty() && m_mbotNonTermMap.empty();
  }

  bool IsLeaf() const {
   std::cout << "Is leaf for non mbot terminal map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const TargetPhraseCollection *GetTargetPhraseCollectionMBOT() const
  {
    return m_mbotTargetPhraseCollection;
  }

  //overwrite Method
  TargetPhraseCollection &GetOrCreateTargetPhraseCollectionMBOT(Word sourceLHS) {

    if (m_mbotTargetPhraseCollection == NULL)
    {
          m_mbotTargetPhraseCollection = new TargetPhraseCollection();
    }
    else
    {}
    return *m_mbotTargetPhraseCollection;
  }

  void Prune(size_t tableLimit)
  {
     std::cout << "Prune non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void PruneMBOT(size_t tableLimit);

  void Sort(size_t tableLimit)
  {
     std::cout << "Sort non mbot Phrase Dictionary NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  void SortMBOT(size_t tableLimit);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChildMBOT(const Word &sourceTerm);

  PhraseDictionaryNodeSCFG *GetOrCreateChild(const Word &sourceNonTerm, const Word &targetNonTerm)
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  PhraseDictionaryNodeMBOT *GetOrCreateChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTerm);

  const PhraseDictionaryNodeSCFG *GetChild(const Word &sourceTerm) const
  {
     std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChildMBOT(const Word &sourceTerm) const;

  const PhraseDictionaryNodeMBOT *GetChild(const Word &sourceNonTerm, const Word &targetNonTerm) const
  {
    std::cout << "Get child NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const PhraseDictionaryNodeMBOT *GetChild(const Word &sourceNonTerm, const std::vector<Word> &targetNonTerm) const;

  const NonTerminalMap & GetNonTerminalMap() const
  {
     std::cout << "Get non mbot non term map NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
  }

  const NonTerminalMapMBOT & GetNonTerminalMapMBOT() const {
    return m_mbotNonTermMap;
  }

  void Clear()
  {
	  std::cout << "Clear non mbot Phrase dictionary node NOT IMPLEMENTED in Phrase Dictionary Node MBOT"<< std::endl;
	  ClearMBOT();
  }

  void ClearMBOT();

  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeMBOT&);

}
