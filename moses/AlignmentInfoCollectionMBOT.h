/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "AlignmentInfoMBOT.h"

#include <set>
#include <cstring>
#include <iostream>
#include <vector>

namespace Moses
{

//Singleton collection of MBOT alignment info objects
//Works the same as Alignment info but uses
class AlignmentInfoCollectionMBOT
{

 public:
  static AlignmentInfoCollectionMBOT &Instance() { return s_instance; }

  // Returns a pointer to an AlignmentInfoMBOT object with the same vectors source-target
  // alignment pairs as given in the argument.  If the collection already
  // contains such an object then returns a pointer to it; otherwise a new
  // one is inserted.
  const AlignmentInfoMBOT *Add(const std::set<std::pair<size_t,size_t> > &, std::set<size_t> allSources, bool isMBOT);

  const AlignmentInfoMBOT &GetEmptyAlignmentInfo() const;

  const std::vector<const AlignmentInfoMBOT*> &GetEmptyAlignmentInfoVector() const;

  const std::vector<const AlignmentInfoMBOT*> *AddVector(std::vector<std::set<std::pair<size_t,size_t> > > &alignmentVector, std::set<size_t> allSources, bool isMBOT);

 private:

  //we make 2 collections :
  //1. set of all alignment pairs
  //2. set of all vectors of alignment pairs
  //set of vectors of target phrases instead of single target phrase

  typedef std::set<AlignmentInfoMBOT, AlignmentInfoOrdererMBOT> AlignmentInfoMBOTSet;
  typedef std::set<std::vector<const AlignmentInfoMBOT*> > AlignmentInfoVectorSet;

  // Only a single static variable should be created.
  AlignmentInfoCollectionMBOT();

  static AlignmentInfoCollectionMBOT s_instance;
  AlignmentInfoMBOTSet m_collection;
  AlignmentInfoVectorSet m_vector_collection;
  const AlignmentInfoMBOT *m_emptyAlignmentInfo;
  const std::vector<const AlignmentInfoMBOT*> *m_emptyAlignmentInfoMBOT;

};
}
