// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <boost/shared_ptr.hpp>

#include <vector>

namespace Moses
{
class ChartTrellisPath;

class ChartTrellisPathList
{
protected:
  typedef std::vector<boost::shared_ptr<const ChartTrellisPath> > CollType;
  CollType m_collection;

public:
  // iters
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;

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
  void clear() {
    m_collection.clear();
  }

  virtual ~ChartTrellisPathList() {}

  std::size_t GetSize() const {
    return m_collection.size();
  }

  //! add a new entry into collection
  void Add(boost::shared_ptr<const ChartTrellisPath> trellisPath) {
    m_collection.push_back(trellisPath);
  }
};

} // namespace

