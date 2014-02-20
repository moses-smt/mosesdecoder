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

#include "AlignmentInfoCollectionMBOT.h"

using namespace std;

namespace Moses
{

AlignmentInfoCollectionMBOT AlignmentInfoCollectionMBOT::s_instance;


AlignmentInfoCollectionMBOT::AlignmentInfoCollectionMBOT()
{
  std::set<std::pair<size_t,size_t> > alignment;
  std::set<size_t> sources;
  std::vector<std::set<std::pair<size_t,size_t> > > alignmentVector;
  m_emptyAlignmentInfo = Add(alignment,sources,1);
  m_emptyAlignmentInfoMBOT = AddVector(alignmentVector,sources,1);
}

const AlignmentInfoMBOT &AlignmentInfoCollectionMBOT::GetEmptyAlignmentInfo() const
{
  return *m_emptyAlignmentInfo;
}

const vector<const AlignmentInfoMBOT*> &AlignmentInfoCollectionMBOT::GetEmptyAlignmentInfoVector() const
{
  return *m_emptyAlignmentInfoMBOT;
}

//Idea : make a shared pointer to AlignmentInfo when adding
const AlignmentInfoMBOT *AlignmentInfoCollectionMBOT::Add(
    const std::set<std::pair<size_t,size_t> > &pairs, std::set<size_t> allSources, bool isMBOT)
{
    std::pair<AlignmentInfoMBOTSet::iterator, bool> ret =
    m_collection.insert(AlignmentInfoMBOT(pairs,allSources,isMBOT));
    return &(*ret.first);
}

const vector<const AlignmentInfoMBOT*> *AlignmentInfoCollectionMBOT::AddVector(
    std::vector<std::set<std::pair<size_t,size_t> > > &alignmentVector, std::set<size_t> allSources, bool isMBOT)
{
	//create vector of AlignmentInfosMBOT before
	std::vector<const AlignmentInfoMBOT*> alignVector;
	std::vector<std::set<std::pair<size_t,size_t> > > :: iterator itr_align_vector;
	for(itr_align_vector = alignmentVector.begin(); itr_align_vector != alignmentVector.end(); itr_align_vector++)
	{
		alignVector.push_back(Add((*itr_align_vector), allSources, isMBOT));
	}
    std::pair<AlignmentInfoVectorSet::iterator, bool> ret =
    m_vector_collection.insert(alignVector);
    return &(*ret.first);
}
}
