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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "FactorCollection.h"
#include "LanguageModel.h"
#include "Util.h"

using namespace std;

namespace Moses
{
FactorCollection FactorCollection::s_instance;

bool FactorCollection::Exists(FactorDirection direction, FactorType factorType, const string &factorString) const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  return m_map.find(factorString) != m_map.end();
}

const Factor *FactorCollection::AddFactor(FactorDirection direction
    , FactorType 			factorType
    , const string 		&factorString)
{
#ifdef WITH_THREADS
  {
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
    Map::const_iterator i = m_map.find(factorString);
    if (i != m_map.end()) return &i->second;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  std::pair<std::string, Factor> to_ins(factorString, Factor());
  std::pair<Map::iterator, bool> ret(m_map.insert(to_ins));
  if (ret.second) {
    Factor &factor = ret.first->second;
    factor.m_id = m_factorId++;
    factor.m_ptrString = &ret.first->first;
  }
  return &ret.first->second;
}

FactorCollection::~FactorCollection()
{
  //FactorSet::iterator iter;
  //for (iter = m_collection.begin() ; iter != m_collection.end() ; iter++)
  //{
  //	delete (*iter);
  //}
}

TO_STRING_BODY(FactorCollection);

// friend
ostream& operator<<(ostream& out, const FactorCollection& factorCollection)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(factorCollection.m_accessLock);
#endif
  for (FactorCollection::Map::const_iterator i = factorCollection.m_map.begin(); i != factorCollection.m_map.end(); ++i) {
    out << i->second;
  }
  return out;
}

}


