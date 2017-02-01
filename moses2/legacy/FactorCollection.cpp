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
#include "util/pool.hh"
#include "util/exception.hh"
#include "../System.h"

using namespace std;

namespace Moses2
{

const Factor *FactorCollection::AddFactor(const StringPiece &factorString,
    const System &system, bool isNonTerminal)
{
  FactorFriend to_ins;
  to_ins.in.m_string = factorString;
  to_ins.in.m_id = (isNonTerminal) ? m_factorIdNonTerminal : m_factorId;
  Set & set = (isNonTerminal) ? m_set : m_setNonTerminal;
  // If we're threaded, hope a read-only lock is sufficient.
#ifdef WITH_THREADS
  {
    // read=lock scope
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
    Set::const_iterator i = set.find(to_ins);
    if (i != set.end()) return &i->in;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif // WITH_THREADS
  std::pair<Set::iterator, bool> ret(set.insert(to_ins));
  if (ret.second) {
    ret.first->in.m_string.set(
      memcpy(m_string_backing.Allocate(factorString.size()),
             factorString.data(), factorString.size()), factorString.size());
    if (isNonTerminal) {
      m_factorIdNonTerminal++;
      UTIL_THROW_IF2(m_factorIdNonTerminal >= moses_MaxNumNonterminals,
                     "Number of non-terminals exceeds maximum size reserved. Adjust parameter moses_MaxNumNonterminals, then recompile");
    } else {
      m_factorId++;
    }
  }

  const Factor *factor = &ret.first->in;

  return factor;
}

const Factor *FactorCollection::GetFactor(const StringPiece &factorString,
    bool isNonTerminal)
{
  FactorFriend to_find;
  to_find.in.m_string = factorString;
  to_find.in.m_id = (isNonTerminal) ? m_factorIdNonTerminal : m_factorId;
  Set & set = (isNonTerminal) ? m_set : m_setNonTerminal;
  {
    // read=lock scope
#ifdef WITH_THREADS
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif // WITH_THREADS
    Set::const_iterator i = set.find(to_find);
    if (i != set.end()) return &i->in;
  }
  return NULL;
}

FactorCollection::~FactorCollection()
{
}

// friend
ostream& operator<<(ostream& out, const FactorCollection& factorCollection)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(factorCollection.m_accessLock);
#endif
  for (FactorCollection::Set::const_iterator i = factorCollection.m_set.begin();
       i != factorCollection.m_set.end(); ++i) {
    out << i->in;
  }
  return out;
}

}

