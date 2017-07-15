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

#include "AlignmentInfo.h"

#include <set>

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

namespace Moses
{

/** Singleton collection of all AlignmentInfo objects.
 *  Used as a cache of all alignment info to save space.
 */
class AlignmentInfoCollection
{
public:
  static AlignmentInfoCollection &Instance() {
    return s_instance;
  }

  /** Returns a pointer to an AlignmentInfo object with the same source-target
    * alignment pairs as given in the argument.  If the collection already
    * contains such an object then returns a pointer to it; otherwise a new
    * one is inserted.
   */
private:
  const AlignmentInfo* Add(AlignmentInfo const& ainfo);

public:
  template<typename ALNREP>
  AlignmentInfo const *
  Add(ALNREP const & aln) {
    return this->Add(AlignmentInfo(aln));
  }

  //! Returns a pointer to an empty AlignmentInfo object.
  const AlignmentInfo &GetEmptyAlignmentInfo() const;

private:
  typedef std::set<AlignmentInfo, AlignmentInfoOrderer> AlignmentInfoSet;


  //! Only a single static variable should be created.
  AlignmentInfoCollection();
  ~AlignmentInfoCollection();

  static AlignmentInfoCollection s_instance;

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

  AlignmentInfoSet m_collection;
  const AlignmentInfo *m_emptyAlignmentInfo;
};

}
