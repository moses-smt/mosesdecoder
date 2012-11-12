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

#ifndef moses_PhraseDictionaryNode_h
#define moses_PhraseDictionaryNode_h

#include <map>
#include <vector>
#include <iterator>
#include "Word.h"
#include "TargetPhraseCollection.h"

namespace Moses
{

class PhraseDictionaryMemory;
class PhraseDictionaryFeature;

/** One node of the PhraseDictionaryMemory structure
 */
class PhraseDictionaryNode
{
  typedef std::map<Word, PhraseDictionaryNode> NodeMap;

  // only these classes are allowed to instantiate this class
  friend class PhraseDictionaryMemory;
  friend class std::map<Word, PhraseDictionaryNode>;

protected:
  NodeMap m_map;
  TargetPhraseCollection *m_targetPhraseCollection;

  PhraseDictionaryNode()
    :m_targetPhraseCollection(NULL)
  {}
public:
  ~PhraseDictionaryNode();

  void Sort(size_t tableLimit);
  PhraseDictionaryNode *GetOrCreateChild(const Word &word);
  const PhraseDictionaryNode *GetChild(const Word &word) const;
  const TargetPhraseCollection *GetTargetPhraseCollection() const {
    return m_targetPhraseCollection;
  }
  TargetPhraseCollection *CreateTargetPhraseCollection() {
    if (m_targetPhraseCollection == NULL)
      m_targetPhraseCollection = new TargetPhraseCollection();
    return m_targetPhraseCollection;
  }

  // iterators
  typedef NodeMap::iterator iterator;
  typedef NodeMap::const_iterator const_iterator;
  const_iterator begin() const {
    return m_map.begin();
  }
  const_iterator end() const {
    return m_map.end();
  }
  iterator begin() {
    return m_map.begin();
  }
  iterator end() {
    return m_map.end();
  }
};

}
#endif
