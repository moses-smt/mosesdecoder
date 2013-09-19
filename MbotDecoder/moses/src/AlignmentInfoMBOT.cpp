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
#include "util/check.hh"
#include "AlignmentInfoMBOT.h"
#include "TypeDef.h"
#include "StaticData.h"

namespace Moses
{

typedef std::vector<size_t> NonTermIndexMap;
typedef boost::shared_ptr<NonTermIndexMap> NonTermIndexMapPointer;

const NonTermIndexMapPointer AlignmentInfoMBOT::GetNonTermIndexMap() const {
    return m_nonTermIndexMap;
  }

void AlignmentInfoMBOT::BuildNonTermIndexMapMBOT(std::set<size_t> allSources, bool isMBOT)
{
  //build a mapping between source and target non-terminals that allows a source non-terminal to
  //be aligned to multiple target non-terminals
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
  m_nonTermIndexMap->resize(maxIndex+1, NOT_FOUND);
  if(!isMBOT)
  {
    size_t i = 0;
      bool PreviousNotSeen = 0;
      bool PreviousSeen = 0;
      std::set<size_t> sources;

      for (p = begin(); p != end(); ++p) {
        if(sources.insert(p->first).second)
        {
            if(PreviousSeen == 1)
           {
                i++;
                PreviousSeen = 0;
           }
            m_nonTermIndexMap->at(p->second) = i++;
            PreviousNotSeen = 1;
        }
        else
        {
           if(PreviousNotSeen == 1)
           {
                i--;
                PreviousNotSeen = 0;
           }

           m_nonTermIndexMap->at(p->second) = i;
           PreviousSeen = 1;
        }
      }
    }
    else
    //if we have an MBot target phrase we must check which other phrases have already been done
    {

        int maxSourceIndex = 0;
        std::set<size_t>::iterator itr_sources;
        for(itr_sources = allSources.begin(); itr_sources != allSources.end(); itr_sources++)
        {
            if((*itr_sources) > maxSourceIndex)
            {
                maxSourceIndex = *itr_sources;
            }

        }
        int target = 0;
        int source = 0;
        std::vector<int> countVector (maxSourceIndex + 1,0);
        std::vector<int> :: iterator itr_countVec;

        for(itr_sources = allSources.begin(); itr_sources != allSources.end(); itr_sources++)
        {
            countVector[*itr_sources] = 1;
        }
        int sum = 0;
        int posCounter = 0;
        for (p = begin(); p != end(); ++p) {
            for(itr_countVec = countVector.begin(); itr_countVec != countVector.end(); itr_countVec++)
            {
                if( posCounter == p->first)
                {
                    target = p->second;
                    source = sum;
                    break;
                }
                sum+=countVector[posCounter];
                posCounter++;
            }
            m_nonTermIndexMap->at(target) = source;
        }
        countVector.clear();
    }
}

  bool compare_target_in_mbot(const std::pair<size_t,size_t> *a, const std::pair<size_t,size_t> *b) {
  if(a->second < b->second)  return true;
  if(a->second == b->second) return (a->first < b->first);
  return false;
}


std::vector< const std::pair<size_t,size_t>* > AlignmentInfoMBOT::GetSortedAlignments() const
{
  std::vector< const std::pair<size_t,size_t>* > ret;

  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter)
  {
    const std::pair<size_t,size_t> &alignPair = *iter;
    ret.push_back(&alignPair);
  }

  const StaticData &staticData = StaticData::Instance();
  WordAlignmentSort wordAlignmentSort = staticData.GetWordAlignmentSort();

  switch (wordAlignmentSort)
  {
    case NoSort:
      break;

    case TargetOrder:
      std::sort(ret.begin(), ret.end(), compare_target_in_mbot);
      break;

    default:
      CHECK(false);
  }

  return ret;

}

std::ostream& operator<<(std::ostream &out, const AlignmentInfoMBOT &alignmentInfo)
{
  AlignmentInfo::const_iterator iter;
  for (iter = alignmentInfo.begin(); iter != alignmentInfo.end(); ++iter) {
    out << iter->first << "-" << iter->second << " ";
  }
  out << std::endl;
  return out;
}

}


