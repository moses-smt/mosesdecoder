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
#include <algorithm>
#include <set>
#include <sstream>
#include "AlignmentInfo.h"
#include "legacy/Util2.h"
#include "util/exception.hh"

namespace Moses2
{

AlignmentInfo::AlignmentInfo(const std::set<std::pair<size_t,size_t> > &pairs)
  : m_collection(pairs)
{
  BuildNonTermIndexMaps();
}

AlignmentInfo::AlignmentInfo(const std::vector<unsigned char> &aln)
{
  assert(aln.size()%2==0);
  for (size_t i = 0; i < aln.size(); i+= 2)
    m_collection.insert(std::make_pair(size_t(aln[i]),size_t(aln[i+1])));
  BuildNonTermIndexMaps();
}

AlignmentInfo::AlignmentInfo(const std::string &str)
{
  std::vector<std::string> points = Tokenize(str, " ");
  std::vector<std::string>::const_iterator iter;
  for (iter = points.begin(); iter != points.end(); iter++) {
    std::vector<size_t> point = Tokenize<size_t>(*iter, "-");
    UTIL_THROW_IF2(point.size() != 2, "Bad format of word alignment point: " << *iter);
    Add(point[0], point[1]);
  }
}

void AlignmentInfo::BuildNonTermIndexMaps()
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
  m_nonTermIndexMap2.resize(maxIndex+1, NOT_FOUND);
  size_t i = 0;
  for (p = begin(); p != end(); ++p) {
    if (m_nonTermIndexMap[p->second] != NOT_FOUND) {
      // 1-to-many. Definitely a set of terminals. Don't bother storing 1-to-1 index map
      m_nonTermIndexMap.clear();
      m_nonTermIndexMap2.clear();
      return;
    }
    m_nonTermIndexMap[p->second] = i++;
    m_nonTermIndexMap2[p->second] = p->first;
  }
}

std::set<size_t> AlignmentInfo::GetAlignmentsForSource(size_t sourcePos) const
{
  std::set<size_t> ret;
  CollType::const_iterator iter;
  for (iter = begin(); iter != end(); ++iter) {
    // const std::pair<size_t,size_t> &align = *iter;
    if (iter->first == sourcePos) {
      ret.insert(iter->second);
    }
  }
  return ret;
}

std::set<size_t> AlignmentInfo::GetAlignmentsForTarget(size_t targetPos) const
{
  std::set<size_t> ret;
  CollType::const_iterator iter;
  for (iter = begin(); iter != end(); ++iter) {
    // const std::pair<size_t,size_t> &align = *iter;
    if (iter->second == targetPos) {
      ret.insert(iter->first);
    }
  }
  return ret;
}


bool
compare_target(std::pair<size_t,size_t> const* a,
               std::pair<size_t,size_t> const* b)
{
  if(a->second < b->second)  return true;
  if(a->second == b->second) return (a->first < b->first);
  return false;
}


std::vector< const std::pair<size_t,size_t>* >
AlignmentInfo::
GetSortedAlignments(WordAlignmentSort SortOrder) const
{
  std::vector< const std::pair<size_t,size_t>* > ret;

  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter) {
    const std::pair<size_t,size_t> &alignPair = *iter;
    ret.push_back(&alignPair);
  }

  switch (SortOrder) {
  case NoSort:
    break;

  case TargetOrder:
    std::sort(ret.begin(), ret.end(), compare_target);
    break;

  default:
    UTIL_THROW(util::Exception, "Unknown word alignment sort option: "
               << SortOrder);
  }

  return ret;

}

std::vector<size_t> AlignmentInfo::GetSourceIndex2PosMap() const
{
  std::set<size_t> sourcePoses;

  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter) {
    size_t sourcePos = iter->first;
    sourcePoses.insert(sourcePos);
  }
  std::vector<size_t> ret(sourcePoses.begin(), sourcePoses.end());
  return ret;
}

std::string AlignmentInfo::Debug(const System &system) const
{
  std::stringstream out;
  out << *this;
  return out.str();
}

std::ostream& operator<<(std::ostream& out, const AlignmentInfo& obj)
{
  AlignmentInfo::const_iterator iter;
  for (iter = obj.begin(); iter != obj.end(); ++iter) {
    out << iter->first << "-" << iter->second << " ";
  }
  return out;
}

}
