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

using namespace std;

namespace Moses
{
FactorCollection FactorCollection::s_instance;

const Factor *FactorCollection::AddFactor(const StringPiece &factorString)
{
// Sorry this is so complicated.  Can't we just require everybody to use Boost >= 1.42?  The issue is that I can't check BOOST_VERSION unless we have Boost.  
#ifdef WITH_THREADS

#if BOOST_VERSION < 104200
  FactorFriend to_ins;
  to_ins.in.m_string.assign(factorString.data(), factorString.size());
#endif // BOOST_VERSION
  { // read=lock scope
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#if BOOST_VERSION >= 104200
    // If this line doesn't compile, upgrade your Boost.  
    Set::const_iterator i = m_set.find(factorString, HashFactor(), EqualsFactor());
#else // BOOST_VERSION
    Set::const_iterator i = m_set.find(to_ins);
#endif // BOOST_VERSION
    if (i != m_set.end()) return &i->in;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#if BOOST_VERSION >= 104200
  FactorFriend to_ins;
  to_ins.in.m_string.assign(factorString.data(), factorString.size());
#endif // BOOST_VERSION

#else // WITH_THREADS

#if BOOST_VERSION >= 104200
  Set::const_iterator i = m_set.find(factorString, HashFactor(), EqualsFactor());
  if (i != m_set.end()) return &i->in;
#endif
  FactorFriend to_ins;
  to_ins.in.m_string.assign(factorString.data(), factorString.size());

#endif // WITH_THREADS
  to_ins.in.m_id = m_factorId;
  std::pair<Set::iterator, bool> ret(m_set.insert(to_ins));
  if (ret.second) {
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


