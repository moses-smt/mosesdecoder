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

#include "AlignmentInfo.h"

#include "TypeDef.h"

namespace Moses
{

void AlignmentInfo::BuildNonTermIndexMap()
{
  if (m_collection.empty()) {
    return;
  }
  const_iterator p = begin();
  size_t maxIndex = p->second;
  for (++p;  p != end(); ++p) {
    if (p->second > maxIndex) {
      maxIndex = p->second;
    }
  }
  m_nonTermIndexMap.resize(maxIndex+1, NOT_FOUND);
  size_t i = 0;
  for (p = begin(); p != end(); ++p) {
    m_nonTermIndexMap[p->second] = i++;
  }
}

std::ostream& operator<<(std::ostream &out, const AlignmentInfo &alignmentInfo)
{
  AlignmentInfo::const_iterator iter;
  for (iter = alignmentInfo.begin(); iter != alignmentInfo.end(); ++iter) {
    out << iter->first << "-" << iter->second << " ";
  }
  return out;
}

}
