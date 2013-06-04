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

#include "AlignmentInfoCollection.h"

namespace Moses
{

AlignmentInfoCollection AlignmentInfoCollection::s_instance;

AlignmentInfoCollection::AlignmentInfoCollection()
{
  std::set<std::pair<size_t,size_t> > pairs;
  m_emptyAlignmentInfo = Add(pairs);
}

AlignmentInfoCollection::~AlignmentInfoCollection()
{}

const AlignmentInfo &AlignmentInfoCollection::GetEmptyAlignmentInfo() const
{
  return *m_emptyAlignmentInfo;
}

const AlignmentInfo *AlignmentInfoCollection::Add(
  const std::set<std::pair<size_t,size_t> > &pairs)
{
  AlignmentInfo pairsAlignmentInfo(pairs);
#ifdef WITH_THREADS
  {
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
    AlignmentInfoSet::const_iterator i = m_collection.find(pairsAlignmentInfo);
    if (i != m_collection.end())
      return &*i;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  std::pair<AlignmentInfoSet::iterator, bool> ret =
    m_collection.insert(pairsAlignmentInfo);
  return &(*ret.first);
}


}
