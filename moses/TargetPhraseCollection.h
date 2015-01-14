// $Id$

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

#ifndef moses_TargetPhraseCollection_h
#define moses_TargetPhraseCollection_h

#include <vector>
#include <iostream>
#include "TargetPhrase.h"
#include "Util.h"

namespace Moses
{

//! a list of target phrases that is translated from the same source phrase
class TargetPhraseCollection
{
protected:
  friend std::ostream& operator<<(std::ostream &, const TargetPhraseCollection &);

  typedef std::vector<const TargetPhrase*> CollType;
  CollType m_collection;

public:
  // iters
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;

  TargetPhrase const*
  operator[](size_t const i) const {
    return m_collection.at(i);
  }

  iterator begin() {
    return m_collection.begin();
  }
  iterator end() {
    return m_collection.end();
  }
  const_iterator begin() const {
    return m_collection.begin();
  }
  const_iterator end() const {
    return m_collection.end();
  }

  TargetPhraseCollection() {
  }

  TargetPhraseCollection(const TargetPhraseCollection &copy);

  virtual ~TargetPhraseCollection() {
    Remove();
  }

  const CollType &GetCollection() const {
    return m_collection;
  }

  //! delete an entry from the collection
  void Remove(const size_t pos) {
    if (pos < m_collection.size()) {
      m_collection.erase(begin() + pos);
    }
  }

  //! return an entry of the collection
  const TargetPhrase* GetTargetPhrase(const size_t pos) const {
    return m_collection[pos];
  }

  //! divide collection into 2 buckets using std::nth_element, the top & bottom according to table limit
  void NthElement(size_t tableLimit);

  //! number of target phrases in this collection
  size_t GetSize() const {
    return m_collection.size();
  }
  //! wether collection has any phrases
  bool IsEmpty() const {
    return m_collection.empty();
  }
  //! add a new entry into collection
  void Add(TargetPhrase *targetPhrase) {
    m_collection.push_back(targetPhrase);
  }

  void Prune(bool adhereTableLimit, size_t tableLimit);
  void Sort(bool adhereTableLimit, size_t tableLimit);

  void Remove() {
    RemoveAllInColl(m_collection);
  }
  void Detach() {
    m_collection.clear();
  }

};


// LEGACY
// DO NOT USE. NOT LEGACY CODE
class TargetPhraseCollectionWithSourcePhrase : public TargetPhraseCollection
{
protected:
  //friend std::ostream& operator<<(std::ostream &, const TargetPhraseCollectionWithSourcePhrase &);

  // TODO boost::ptr_vector
  std::vector<Phrase> m_sourcePhrases;

public:
  const std::vector<Phrase> &GetSourcePhrases() const {
    return m_sourcePhrases;
  }

  void Add(TargetPhrase *targetPhrase);
  void Add(TargetPhrase *targetPhrase, const Phrase &sourcePhrase);
};

struct CompareTargetPhrase {
  bool operator() (const TargetPhrase *a, const TargetPhrase *b) const;
  bool operator() (const TargetPhrase &a, const TargetPhrase &b) const;
};

}

#endif
