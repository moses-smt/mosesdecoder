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

#include <boost/version.hpp>
#ifdef WITH_THREADS
#include <boost/thread/locks.hpp>
#endif
#include <ostream>
#include <string>
#include "FactorCollection.h"
#include "Util.h"
#include "util/pool.hh"

using namespace std;

namespace Moses
{
FactorCollection FactorCollection::s_instance;

const Factor *FactorCollection::AddFactor(const StringPiece &factorString)
{
  FactorFriend to_ins;
  to_ins.in.m_string = factorString;
  to_ins.in.m_id = m_factorId;
  // If we're threaded, hope a read-only lock is sufficient.
#ifdef WITH_THREADS
  {
    // read=lock scope
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
    Set::const_iterator i = m_set.find(to_ins);
    if (i != m_set.end()) return &i->in;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif // WITH_THREADS
  std::pair<Set::iterator, bool> ret(m_set.insert(to_ins));
  if (ret.second) {
    ret.first->in.m_string.set(
      memcpy(m_string_backing.Allocate(factorString.size()), factorString.data(), factorString.size()),
      factorString.size());
    m_factorId++;
  }
  return &ret.first->in;
}

FactorCollection::~FactorCollection() {}

TO_STRING_BODY(FactorCollection);

// friend
ostream& operator<<(ostream& out, const FactorCollection& factorCollection)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(factorCollection.m_accessLock);
#endif
  for (FactorCollection::Set::const_iterator i = factorCollection.m_set.begin(); i != factorCollection.m_set.end(); ++i) {
    out << i->in;
  }
  return out;
}

}


