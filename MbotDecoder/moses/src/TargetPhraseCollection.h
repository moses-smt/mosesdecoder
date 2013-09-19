// $Id: TargetPhraseCollection.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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
#include "TargetPhrase.h"
#include "TargetPhraseMBOT.h"
#include "Util.h"

namespace Moses
{

//! a list of target phrases that is translated from the same source phrase
class TargetPhraseCollection
{
protected:

  std::vector<TargetPhrase*> m_collection;

public:
  // iters
  typedef std::vector<TargetPhrase*>::iterator iterator;
  typedef std::vector<TargetPhrase*>::const_iterator const_iterator;

  iterator begin() {
    //std::cout << "TPC TOUCH : USING ITERATOR " << std::endl;
    //std::cout << "Address of Target Phrase : " << &(*(m_collection.begin())) << std::endl;
    return m_collection.begin();
  }
  iterator end() {
       //std::cout << "TPC TOUCH : USING ITERATOR " << std::endl;
    return m_collection.end();
  }
  const_iterator begin() const {
       //std::cout << "TPC TOUCH : USING ITERATOR " << std::endl;
    return m_collection.begin();
  }
  const_iterator end() const {
       //std::cout << "TPC TOUCH: USING ITERATOR " << std::endl;
    return m_collection.end();
  }

  ~TargetPhraseCollection() {
	//MBOT : Beware : Each target phrase has to be casted to MBOT and then deleted
    RemoveAllInColl(m_collection);
  }

  //copy constructor for target phrases
  /*void CopyTargetPhraseCollection(std::vector<TargetPhrase*> tpc) {
	m_collection = tpc;
  }*/

  const std::vector<TargetPhrase*> &GetCollection() const { return m_collection; }

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
    TargetPhraseMBOT *mbotTargetPhrase = static_cast<TargetPhraseMBOT*>(targetPhrase);
    //std::cout << "ADDING TARGET  PHRASE " << *mbotTargetPhrase << std::endl;
	CHECK(targetPhrase != NULL);
    m_collection.push_back(mbotTargetPhrase);
  }

  void Prune(bool adhereTableLimit, size_t tableLimit);
  void Sort(bool adhereTableLimit, size_t tableLimit);

};

}

#endif
