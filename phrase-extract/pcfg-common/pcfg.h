/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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
#ifndef PCFG_PCFG_H_
#define PCFG_PCFG_H_

#include "typedef.h"

#include <istream>
#include <map>
#include <ostream>
#include <vector>

namespace Moses
{
namespace PCFG
{

class Pcfg
{
public:
  typedef std::vector<std::size_t> Key;
  typedef std::map<Key, double> Map;
  typedef Map::iterator iterator;
  typedef Map::const_iterator const_iterator;

  Pcfg() {}

  iterator begin() {
    return rules_.begin();
  }
  const_iterator begin() const {
    return rules_.begin();
  }

  iterator end() {
    return rules_.end();
  }
  const_iterator end() const {
    return rules_.end();
  }

  void Add(const Key &, double);
  bool Lookup(const Key &, double &) const;
  void Read(std::istream &, Vocabulary &);
  void Write(const Vocabulary &, std::ostream &) const;

private:
  Map rules_;
};

}  // namespace PCFG
}  // namespace Moses

#endif
